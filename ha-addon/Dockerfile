FROM debian:bullseye-slim

# Install Python, pip, and jq
RUN apt-get update && \
    apt-get install -y python3 python3-pip jq && \
    pip3 install jinja2 && \
    apt-get clean

WORKDIR /app

COPY run.sh /app/run.sh
COPY dashboard_template.yaml /app/dashboard_template.yaml

CMD ["/bin/bash", "/app/run.sh"]