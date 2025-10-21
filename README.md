# trading-bot

# Trading bot using Cross-sectional reference and moving average

# Usage : 
use fetch_data.py to pull the necessary data. use the tech_stocks.txt to change necessary tickers. this program uses yfinance library and pandas dataframe to save fetched data. strategy.cpp is a basic implementation of the strategy and backtesting. this strategy is backtested on 1 year data from yahoo finance.

# Strategy : 
We basically go long or short the top 10% stocks in terms of absolute signal strength. signal here is the momentum score which is average price in last 50 days / average price in last 200 days

