FROM python:3.12-slim

WORKDIR /app
COPY gateway.py rate_limiter.py ./

EXPOSE 8090
ENTRYPOINT ["python3", "gateway.py"]
