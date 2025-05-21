FROM python:3.11-slim

RUN apt-get update && apt-get install -y jq && pip install jinja2

WORKDIR /app
COPY run.sh /app/run.sh
COPY dashboard_template.yaml /app/dashboard_template.yaml

CMD [ "/bin/bash", "/app/run.sh" ]