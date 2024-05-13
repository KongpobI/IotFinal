import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.linear_model import LinearRegression
from sklearn.metrics import mean_squared_error
import joblib

# Load the data
data = pd.read_csv("E:\IOT_slides\data_ayud.csv")  # Replace "your_data_file.csv" with the path to your data file

# Extract features (X) and target variable (y)
X = data.drop(columns=["time"])  # Features (excluding "time" column)
y = data[["humid", "pres", "temp"]]  # Target variables ("temp", "humid", "pres" columns)

# Create input-output pairs
X_train = []
y_train = []

# Iterate over the data to create input-output pairs
for i in range(len(data) - 5):
    X_train.append(X.iloc[i:i+5])  # Previous 5 rows as input
    y_train.append(y.iloc[i+5])    # 6th row as output

# Convert lists to arrays
X_train = np.array(X_train)
y_train = np.array(y_train)

# Display the shape of X_train and y_train
print("X_train shape:", X_train.shape)
print("y_train shape:", y_train.shape)

# Split the data into training and testing sets (80% train, 20% test)
X_train, X_test, y_train, y_test = train_test_split(X_train, y_train, test_size=0.2, random_state=42)

# Fit a linear regression model
model = LinearRegression()
model.fit(X_train.reshape(X_train.shape[0], -1), y_train)  # Reshape X_train to 2D array

# Make predictions on the test set
y_pred = model.predict(X_test.reshape(X_test.shape[0], -1))  # Reshape X_test to 2D array

# Calculate the mean squared error
mse = mean_squared_error(y_test, y_pred)
print("\nMean Squared Error:", mse)

# Predict the next values based on the last 5 rows of the dataset
last_5_rows = X.iloc[-5:].values.reshape(1, -1)  # Reshape the last 5 rows to 1D array
next_values = model.predict(last_5_rows)
print("\nPredicted next values (humid, pres, temp) based on last 5 rows:", next_values)

# Save the trained model
joblib.dump(model, 'regression_model.pkl')
print('Model dumped')

# Save the list of columns used in training
regression_model_columns = list(X_train)
joblib.dump(regression_model_columns, 'regression_model_columns.pkl')