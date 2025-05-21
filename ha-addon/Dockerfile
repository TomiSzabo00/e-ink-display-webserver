FROM python:3.11-slim

RUN pip install jinja2

WORKDIR /app
COPY run.sh /app/run.sh
COPY dashboard_template.yaml /app/dashboard_template.yaml

CMD [ "/bin/bash", "/app/run.sh" ]