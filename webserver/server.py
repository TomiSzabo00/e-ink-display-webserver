from flask import Flask, request, render_template, send_from_directory, jsonify, Response
import os
from PIL import Image
import json

UPLOAD_FOLDER = "uploads"
TEMPLATE_FOLDER = "templates"
ALLOWED_EXTENSIONS = {"png", "jpg", "jpeg", "bmp"}
STATUS_FILE = "status.jsonl"  # JSON Lines format

app = Flask(__name__)
app.config["UPLOAD_FOLDER"] = UPLOAD_FOLDER
app.config["TEMPLATE_FOLDER"] = TEMPLATE_FOLDER

# Define the 3-color palette (black, white, red)
PALETTE = [(0, 0, 0), (255, 255, 255), (255, 0, 0)]  # Black, White, Red

# Ensure the upload folder exists
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

def allowed_file(filename):
    return "." in filename and filename.rsplit(".", 1)[1].lower() in ALLOWED_EXTENSIONS

@app.route("/", methods=["GET", "POST"])
def upload_file():
    if request.method == "POST":
        if "file" not in request.files:
            return "No file part", 400
        file = request.files["file"]
        if file.filename == "":
            return "No selected file", 400
        if file and allowed_file(file.filename):
            filepath = os.path.join(app.config["UPLOAD_FOLDER"], "latest.png")
            file.save(filepath)
            process_image(filepath)
            return "Upload successful!", 200
    return render_template("index.html")

@app.route("/image/latest")
def serve_latest_image():
    return send_from_directory(app.config["UPLOAD_FOLDER"], "latest.png")

@app.route("/image/processed")
def serve_processed_image():
    filepath = os.path.join(app.config["UPLOAD_FOLDER"], "processed.png")
    if not os.path.exists(filepath):
        return "No processed image yet", 404
    return send_from_directory(app.config["UPLOAD_FOLDER"], "processed.png")

@app.route("/image/border")
def serve_border_image():
    return send_from_directory(app.config["TEMPLATE_FOLDER"], 'eink_border.png')

@app.route("/image/preview")
def serve_preview_image():
    filepath = os.path.join(app.config["UPLOAD_FOLDER"], "preview.png")
    if not os.path.exists(filepath):
        return "No preview image yet", 404
    return send_from_directory(app.config["UPLOAD_FOLDER"], "preview.png")

@app.route('/process-image', methods=['POST'])
def process_uploaded_image():
    if 'file' not in request.files:
        return jsonify({"error": "No file part"}), 400

    file = request.files['file']
    if file.filename == '':
        return jsonify({"error": "No selected file"}), 400

    preview_file_name = "preview.png"
    filepath = os.path.join(app.config['UPLOAD_FOLDER'], preview_file_name)
    file.save(filepath)

    processed_image_path = process_image(filepath, output_path=preview_file_name)
    return jsonify({"processedImageUrl": f"/image/preview"}), 200

@app.route("/image/buffers")
def serve_buffers():
    filepath = os.path.join(app.config["UPLOAD_FOLDER"], "processed.png")
    if not os.path.exists(filepath):
        return "No processed image yet", 404

    black_buffer, red_buffer = process_image_to_buffers(filepath)

    # Pack both buffers into a single response (black first, then red)
    response_data = bytes(black_buffer) + bytes(red_buffer)

    return Response(
        response_data,
        mimetype="application/octet-stream",
        headers={
            "Content-Disposition": "attachment;filename=buffers.bin",
            "X-Buffer-Size": str(len(black_buffer)),  # Helps ESP32 split the response
        },
    )

def process_image(image_path, output_path="processed.png"):
    """Process the image to match E-Ink display format (296x128, black-white-red)"""
    img = Image.open(image_path).convert("RGBA")  # Open as RGBA to handle transparency
    img = img.resize((296, 128))  # Resize to match display resolution

    def closest_color(pixel):
        """Find the closest match in the 3-color palette"""
        r, g, b, a = pixel  # Extract RGBA values
        if a < 128:  # If pixel is transparent (alpha < 128), treat it as white
            return (255, 255, 255)

        # Find the closest color in the palette
        return min(PALETTE, key=lambda color: sum((c1 - c2) ** 2 for c1, c2 in zip((r, g, b), color)))

    pixels = list(img.getdata())
    new_pixels = [closest_color(pixel) for pixel in pixels]

    # Create new image with the processed colors
    new_img = Image.new("RGB", (296, 128))
    new_img.putdata(new_pixels)

    processed_path = os.path.join(app.config["UPLOAD_FOLDER"], output_path)
    new_img.save(processed_path)
    return processed_path

def process_image_to_buffers(image_path):
    """Process image and return black/red buffers (1-bit per pixel)"""
    img = Image.open(image_path).convert("RGBA")
    img = img.resize((296, 128))  # Resize to match display resolution

    width, height = img.size
    buffer_size = (width * height + 7) // 8  # Calculate bytes needed (1 bit per pixel)

    # Initialize buffers (all zeros)
    black_buffer = bytearray(buffer_size)
    red_buffer = bytearray(buffer_size)

    for y in range(height):
        for x in range(width):
            pixel = img.getpixel((x, y))
            r, g, b, a = pixel
            if a < 128:  # Transparent → white (no action, buffers are pre-zeroed)
                continue

            # Find closest color
            closest = min(PALETTE, key=lambda color: sum((c1 - c2) ** 2 for c1, c2 in zip((r, g, b), color)))

            # Calculate buffer position
            idx = y * width + x
            byte_pos = idx // 8
            bit_pos = 7 - (idx % 8)  # MSB first

            if closest == (0, 0, 0):  # Black
                black_buffer[byte_pos] |= (1 << bit_pos)
            elif closest == (255, 0, 0):  # Red
                red_buffer[byte_pos] |= (1 << bit_pos)
            # White: do nothing

    return black_buffer, red_buffer

@app.route('/status', methods=['POST'])
def receive_status():
    data = request.get_json()
    if not data:
        return jsonify({"error": "No JSON payload"}), 400

    timestamp = data.get("timestamp")
    battery = data.get("battery")

    if timestamp is None or battery is None:
        return jsonify({"error": "Missing 'timestamp' or 'battery'"}), 400

    voltage = battery.get("voltage")
    soc = battery.get("SoC")

    if voltage is None or soc is None:
        return jsonify({"error": "Missing 'voltage' or 'SoC' in battery object"}), 400

    status_data = {
        "timestamp": timestamp,
        "battery": {
            "voltage": voltage,
            "SoC": soc
        }
    }

    # Append the new status as a line in the file
    with open(STATUS_FILE, "a") as f:
        f.write(json.dumps(status_data) + "\n")

    return jsonify({"message": "Status received", "status": status_data}), 200

@app.route("/shouldUpdate", methods=["GET"])
def should_update():
    """
    Example: /shouldUpdate?lastAccess=1716220000
    Returns: true or false (as JSON boolean)
    """
    latest_path = os.path.join(app.config["UPLOAD_FOLDER"], "latest.png")
    if not os.path.exists(latest_path):
        return jsonify(False)

    last_modified = int(os.path.getmtime(latest_path))
    last_access = request.args.get("lastAccess", type=int)

    if last_access is None:
        # If not provided, always suggest update
        return jsonify(True)
    else:
        return jsonify(last_modified > last_access)

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5001)