#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iomanip>


// this is only knobs for the strategy
// these numbers worked the best
const int SMA_SHORT = 50;
const int SMA_LONG = 200;
const double PORTFOLIO_SIZE_PERCENT = 0.10; 

using namespace std;

// holds all my data for a single stock on a single day
struct DailyData {
    // From CSV
    string date;
    double open;
    double high;
    double low;
    double close;
    long long volume;
    double dividends;
    double stock_splits;
    double daily_return = 0.0;
    double sma_short = 0.0;
    double sma_long = 0.0;
    double momentum_score = 0.0;
};

// container for all stock data, keyed by ticker
// this wrapper/decorater was added by gpt when done to pretty up code
using StockDataMap = map<string, vector<DailyData>>;

void load_csv_data(const string& filename, StockDataMap& stock_data);
void calculate_metrics(StockDataMap& stock_data);
void run_backtest(const StockDataMap& stock_data);
void print_performance_metrics(const vector<double>& returns);

int main() {
    cout << "Starting Quantitative Backtesting Engine..." << endl;

    StockDataMap stock_data;
    load_csv_data("tech_stocks_data.csv", stock_data);
    if (stock_data.empty()) {
        return 1; // Exit if data loading failed
    }

    calculate_metrics(stock_data);

    run_backtest(stock_data);

    return 0;
}

// --- Function Implementations ---

/**
 * @brief Loads and parses the stock data from the specified CSV file.
 * @param filename The path to the CSV file.
 * @param stock_data The map to populate with stock data.
 */
void load_csv_data(const string& filename, StockDataMap& stock_data) {
    cout << "Loading data from '" << filename << "'..." << endl;
    ifstream file(filename);

    
    string line;
    getline(file, line); // Skip header

    while (getline(file, line)) {
        stringstream ss(line);
        string field;
        vector<string> fields;
        while (getline(ss, field, ',')) {
            fields.push_back(field);
        }


        try {
            DailyData day;
            day.date = fields[0].substr(0, 10); // Keep only YYYY-MM-DD
            string ticker = fields[1];
            day.open = stod(fields[2]);
            day.high = stod(fields[3]);
            day.low = stod(fields[4]);
            day.close = stod(fields[5]);
            day.volume = stoll(fields[6]);
            day.dividends = stod(fields[7]);
            day.stock_splits = stod(fields[8]);
            
            stock_data[ticker].push_back(day);
        } catch (const invalid_argument& e) {
            // Silently skip rows with conversion errors
            // might be ipo or something - idk figure out later - gpt fixed this
            cout << "error prone line skipped" << endl;
            // print line
            cout << line << endl;
        }
    }
    file.close();
    cout << "Data loaded successfully for " << stock_data.size() << " tickers." << endl;
}

/**
 * @brief Calculates daily returns, SMAs, and momentum scores for all stocks.
 * @param stock_data The map of stock data to be enriched.
 */
void calculate_metrics(StockDataMap& stock_data) {
    cout << "calculating signals" << endl;

    for (auto& pair : stock_data) {
        auto& history = pair.second;
        if (history.empty()) continue;

        // Calculate Daily Returns
        for (size_t i = 1; i < history.size(); ++i) {
            if (history[i-1].close > 0) {
                history[i].daily_return = (history[i].close / history[i-1].close) - 1.0;
            }
        }

        // Calculate SMAs using a rolling sum for efficiency
        // this is done by following options playbook and some random websites
        
        double short_sum = 0.0;
        double long_sum = 0.0;
        for (size_t i = 0; i < history.size(); ++i) {
            short_sum += history[i].close;
            long_sum += history[i].close;

            if (i >= SMA_SHORT) {
                short_sum -= history[i - SMA_SHORT].close;
                history[i].sma_short = short_sum / SMA_SHORT;
            }
            if (i >= SMA_LONG) {
                long_sum -= history[i - SMA_LONG].close;
                history[i].sma_long = long_sum / SMA_LONG;
            }
        }
        
        // Calculate Momentum Score
        for (size_t i = SMA_LONG; i < history.size(); ++i) {
             if (history[i].sma_long > 0) {
                history[i].momentum_score = (history[i].sma_short / history[i].sma_long) - 1.0;
             }
        }
    }
    cout << "Indicators calculated." << endl;
}

/**
 * @brief Runs the backtesting simulation based on the pre-calculated metrics.
 * @param stock_data The fully enriched map of stock data.
 */
void run_backtest(const StockDataMap& stock_data) {
    cout << "\n--- Running Backtest Simulation ---" << endl;
    // several parts of this are scoured from the stackoverflow - in pieces
    // and then tailored for this code
    
    // Create a date-indexed map for easy daily lookups
    map<string, vector<const DailyData*>> date_map;
    vector<string> trading_days;
    for (const auto& pair : stock_data) {
        for (const auto& day : pair.second) {
            date_map[day.date].push_back(&day);
        }
    }
    for(const auto& pair : date_map){
        trading_days.push_back(pair.first);
    }
    sort(trading_days.begin(), trading_days.end());

    vector<string> my_portfolio;
    vector<double> strategy_returns;

    // Main backtesting loop
    for (size_t i = SMA_LONG; i < trading_days.size(); ++i) {
        const string& today_str = trading_days[i];
        const string& tomorrow_str = (i + 1 < trading_days.size()) ? trading_days[i+1] : "";
        
        int current_month = stoi(today_str.substr(5, 2));
        int next_day_month = tomorrow_str.empty() ? -1 : stoi(tomorrow_str.substr(5, 2));

        // Rebalance on the last trading day of the month
        if (current_month != next_day_month || tomorrow_str.empty()) {
            vector<pair<double, string>> ranked_stocks;
            
            // Get tickers and their scores for today
            for (const auto& pair : stock_data) {
                const auto& history = pair.second;
                // Find today's data for this stock
                auto it = find_if(history.begin(), history.end(), [&](const DailyData& d){
                    return d.date == today_str;
                });

                if (it != history.end() && it->momentum_score != 0.0) {
                    ranked_stocks.push_back({it->momentum_score, pair.first});
                }
            }

            sort(ranked_stocks.rbegin(), ranked_stocks.rend());
            
            my_portfolio.clear();
            int num_to_select = static_cast<int>(ranked_stocks.size() * PORTFOLIO_SIZE_PERCENT);
            for (int j = 0; j < num_to_select && j < ranked_stocks.size(); ++j) {
                my_portfolio.push_back(ranked_stocks[j].second);
            }
        }
        
        if (!my_portfolio.empty()) {
            double portfolio_daily_return = 0.0;
            int valid_stocks_in_portfolio = 0;

            for (const auto& ticker : my_portfolio) {
                const auto& history = stock_data.at(ticker);
                 auto it = find_if(history.begin(), history.end(), [&](const DailyData& d){
                    return d.date == today_str;
                });
                if (it != history.end()) {
                    portfolio_daily_return += it->daily_return;
                    valid_stocks_in_portfolio++;
                }
            }
            if(valid_stocks_in_portfolio > 0) {
                 strategy_returns.push_back(portfolio_daily_return / valid_stocks_in_portfolio);
            }
        }
    }

    // 4. Print final performance
    print_performance_metrics(strategy_returns);
}

/**
 * @brief Calculates and prints the final performance summary.
 * @param returns A vector of the strategy's daily returns.
 */
void print_performance_metrics(const vector<double>& returns) {
    // this is copy-paste stuff
    if (returns.empty()) {
        cout << "No trades were made. Cannot calculate performance." << endl;
        return;
    }

    cout << "\n--- Backtest Performance Results ---" << endl;

    double total_return = 1.0;
    for (double r : returns) {
        total_return *= (1.0 + r);
    }
    total_return -= 1.0;

    double mean_daily_return = accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    double annualized_return = pow(1.0 + mean_daily_return, 252) - 1.0;

    double sq_sum = inner_product(returns.begin(), returns.end(), returns.begin(), 0.0);
    double stdev_daily = sqrt(sq_sum / returns.size() - mean_daily_return * mean_daily_return);
    double annualized_volatility = stdev_daily * sqrt(252);
    
    double sharpe_ratio = (annualized_volatility > 0) ? (annualized_return / annualized_volatility) : 0.0;
    
    // Max Drawdown Calculation
    double peak = 1.0;
    double max_drawdown = 0.0;
    double cumulative_return = 1.0;
    for (double r : returns) {
        cumulative_return *= (1.0 + r);
        peak = max(peak, cumulative_return);
        double drawdown = (peak - cumulative_return) / peak;
        max_drawdown = max(max_drawdown, drawdown);
    }

    cout << fixed << setprecision(4);
    cout << "Total Trading Days:      " << returns.size() << endl;
    cout << "Cumulative Return:       " << total_return * 100.0 << "%" << endl;
    cout << "Annualized Return:       " << annualized_return * 100.0 << "%" << endl;
    cout << "Annualized Volatility:   " << annualized_volatility * 100.0 << "%" << endl;
    cout << "Sharpe Ratio:            " << sharpe_ratio << endl;
    cout << "Maximum Drawdown:        " << max_drawdown * 100.0 << "%" << endl;
    cout << "------------------------------------" << endl;
}
