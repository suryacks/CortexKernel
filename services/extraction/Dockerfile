FROM python:3.12-slim

WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY schemas.py extraction_client.py mock_extraction_client.py app.py ./

EXPOSE 8082
ENV EXTRACTION_BACKEND=mock
CMD ["uvicorn", "app:app", "--host", "0.0.0.0", "--port", "8082"]
