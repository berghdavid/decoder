"""
API for server
"""
from datetime import datetime
from typing import List
from bottle import get, post, put, delete, request, response, hook


# API endpoints

@post('/')
def get_ping():
    """ Test endpoint """
    print(f"Request: {request}")
    response.content_type = "application/json"
    response.status = 200
    return 'Pong!'

@get('/')
def get_ping():
    """ Test endpoint """
    print(f"Request: {request}")
    response.content_type = "application/json"
    response.status = 200
    return 'Pong!'
