# yfinance
import yfinance as yf
import pandas as pd
# open tickers_data.csv to write to
# !NOTE : REMOVES ALL DATA FROM FILE : OPENING IN WRITE MODE
with open("tech_stocks_data.csv", "w", newline="") as file :
    # data is by default with adjusted close prices
    def write_ticker_data (ticker) :
        # tell will insert per header
        data = yf.Ticker(ticker).history(period="1y", interval="1d")
        data.insert(0, "Ticker", ticker)
        data.to_csv(file, header=file.tell() == 0)

    # open tickers to read from
    def read_tickers () :
        with open("tech_stocks.txt", "r") as file :
            tickers = file.read().splitlines()
        print ("exiting read_tickers")
        return tickers
        
    def main () :
        print ("starting fetch engines : ")
        tickers = read_tickers()
        cnt = 0
        for ticker in tickers :
            print (f"fetching data for {ticker} : {cnt+1}/{len(tickers)}")
            write_ticker_data(ticker)
            cnt += 1
        print("fetch complete")
        # filter data for each ticker

    
    print ("Executing fetch_data.py")
    main()
