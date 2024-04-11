#!/bin/bash

# The path where the virtual environment will be created
ENV_PATH="./venv"

# The name of the environment to use
ENV_NAME="sqlitepb"

# Activate environment if it exists, else create and activate
if [ -d "$ENV_PATH" ]; then
  echo "Activating existing environment '$ENV_NAME'"
else
  echo "Creating new environment '$ENV_NAME'"
  python3 -m venv $ENV_PATH
  if [ $? -ne 0 ]; then
    echo "Failed to create virtualenv '$ENV_NAME'"
    exit 1
  fi
  # Activate the environment
  source $ENV_PATH/bin/activate
  if [ $? -ne 0 ]; then
    echo "Failed to activate virtualenv '$ENV_NAME'"
    exit 1
  fi 
fi

# Activate the environment if it was not newly created
if [ "$VIRTUAL_ENV" != "$PWD/$ENV_PATH" ]; then
  source $ENV_PATH/bin/activate
  if [ $? -ne 0 ]; then
    echo "Failed to activate virtualenv '$ENV_NAME'"
    exit 1
  fi
  # Install requirements
  pip3 install -r requirements.txt
  if [ $? -ne 0 ]; then
    echo "Failed to install requirements"
    exit 1
  fi
fi



# Deactivate the environment
echo "Deactivating the environment '$ENV_NAME'"
deactivate

