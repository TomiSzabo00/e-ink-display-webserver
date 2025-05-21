#!/bin/bash

# Pull config from options.json (mounted automatically)
CONFIG_PATH="/data/options.json"

BACKEND_HOST=$(jq -r '.backend_host' $CONFIG_PATH)
BACKEND_PORT=$(jq -r '.backend_port' $CONFIG_PATH)

# Use Jinja2 to render template
python3 -c "
from jinja2 import Template
with open('/app/dashboard_template.yaml') as f:
    tmpl = Template(f.read())
with open('/share/eink_dashboard.yaml', 'w') as out:
    out.write(tmpl.render(host='$BACKEND_HOST', port=$BACKEND_PORT))
"

echo "Lovelace dashboard written to /share/eink_dashboard.yaml"