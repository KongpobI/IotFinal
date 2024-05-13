from flask import Flask, request, jsonify
import joblib
import traceback
import pandas as pd 
import numpy as np
from dotenv import load_dotenv
import os
from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import ASYNCHRONOUS

# Load the trained model and model columns
lr = joblib.load("regression_model.pkl")
model_columns = joblib.load("regression_model_columns.pkl")

# Initialize Flask app
app = Flask(__name__)

# Load environment variables from ".env"
load_dotenv()



# InfluxDB config
BUCKET = os.environ.get('INFLUXDB_BUCKET')
print("connecting to", os.environ.get('INFLUXDB_URL'))
client = InfluxDBClient(
    url=str(os.environ.get('INFLUXDB_URL')),
    token=str(os.environ.get('INFLUXDB_TOKEN')),
    org=os.environ.get('INFLUXDB_ORG')
)
write_api = client.write_api()



# Define API endpoint for predictions
@app.route('/predict', methods=['POST'])
def predict():
    if lr:
        try:
            json_ = request.json
            print(json_)

            # Extract data from JSON and convert to list of lists
            input_data = []
            for entry in json_:
                temp = entry.get('temp')
                humid = entry.get('humid')
                pres = entry.get('pres')
                if temp is not None and humid is not None and pres is not None:
                    input_data.append([humid, pres, temp])

            print("Input data for prediction:")
            print(input_data)
            
            input_data_array = np.array(input_data)
            input_data_reshaped = input_data_array.reshape(1, -1)
            # Make predictions using the model
            prediction = lr.predict(input_data_reshaped)
            print("Predicted values:")
            print(prediction)

            # Create a Point object for writing to InfluxDB
            point = Point("predict_value")
            # Access the predicted values
            predicted_values = prediction[0]
            
            # Define labels for predicted values
            labels = ["Humidity", "Pressure", "Temperature"]

            # Add the predicted values as fields to the Point object
            for i, value in enumerate(predicted_values):
                point.field(f"Predicted_{labels[i]}", value)
            # Write the Point object to InfluxDB
            write_api.write(BUCKET, os.environ.get('INFLUXDB_ORG'), point)
            return jsonify({'prediction': prediction.tolist()}),200

        except Exception as e:
            return jsonify({'trace': traceback.format_exc()}),400
    else:
        print('Train the model first')
        return 'No model here to use'

# Run the Flask app
if __name__ == '__main__':
    app.run()
