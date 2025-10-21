# yfinance for fetching data, pandas for data manipulation
import yfinance as yf
import pandas as pd

def read_tickers(filename="tech_stocks.txt"):
    """Reads a list of tickers from a text file."""
    try:
        with open(filename, "r") as file:
            tickers = file.read().splitlines()
        print(f"Successfully read {len(tickers)} tickers from '{filename}'")
        return tickers
    except FileNotFoundError:
        print(f"Error: The file '{filename}' was not found.")
        return []

def main():
    """
    Main function to fetch data, calculate returns, and save to CSV.
    """
    print("--- Starting Data Fetching and Processing ---")
    
    # 1. Read the list of tickers
    tickers = read_tickers()
    if not tickers:
        print("No tickers to process. Exiting.")
        return

    # 2. Fetch data for ALL tickers at once.
    # yf.download() is much more efficient for multiple tickers.
    # Let's get 5 years of data for more robust backtesting.
    print(f"\nFetching 5 years of daily data for {len(tickers)} tickers...")
    all_data = yf.download(tickers, period="5y", interval="1d")

    # 3. Reformat the DataFrame from wide to long format.
    # The downloaded data has a multi-level column index. Stacking turns the
    # ticker level ('AAPL', 'MSFT', etc.) into a row index.
    all_data_stacked = all_data.stack(level=1).reset_index()
    all_data_stacked.rename(columns={'level_1': 'Ticker'}, inplace=True)

    # 4. Calculate the daily return for each ticker.
    # We group by 'Ticker' to ensure the calculation is contained within each stock's history.
    # .pct_change() is the pandas function for simple return: (new - old) / old
    print("\nCalculating daily returns for each stock...")
    all_data_stacked['Daily_Return'] = all_data_stacked.groupby('Ticker')['Close'].pct_change()

    # The first entry for each stock will be NaN (Not a Number). We fill it with 0.
    all_data_stacked['Daily_Return'].fillna(0.0, inplace=True)

    # 5. Save the final, complete DataFrame to a single CSV file.
    output_filename = "tech_stocks_processed.csv"
    all_data_stacked.to_csv(output_filename, index=False, float_format='%.6f')

    print(f"\nProcessing complete. Final data saved to '{output_filename}'")

    # 6. Display a sample to verify the result
    print("\nSample of the final data for 'MSFT':")
    print(all_data_stacked[all_data_stacked['Ticker'] == 'MSFT'].head())

if __name__ == "__main__":
    main()
