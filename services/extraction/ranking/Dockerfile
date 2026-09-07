FROM python:3.12-slim

WORKDIR /app
COPY bandit.py persistence.py service.py ./

EXPOSE 8081
ENTRYPOINT ["python3", "service.py"]
