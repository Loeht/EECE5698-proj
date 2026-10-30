#define _DEFAULT_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SHORT_WINDOW  9
#define LONG_WINDOW   21
#define ALMA_OFFSET   0.85
#define ALMA_SIGMA    6.0
#define START_PRICE   100.00
#define MAX_MOVE      0.01
#define NUM_TICKS     10000
#define SEED          42
#define START_CASH    1000.00

void alma_cross_hw(float* price, int* trade, int n);

double uniform(void)
{
    return random() / ((double)RAND_MAX + 1.0);
}

double random_price_delta(void)
{
    double u = (uniform() + uniform() + uniform()) / 3.0;
    return (2.0 * u - 1.0) * MAX_MOVE;
}

double alma(float* price, int end, int n)
{
    double m = ALMA_OFFSET * (n - 1);
    double s = n / ALMA_SIGMA;

    double acc = 0.0;
    double wsum = 0.0;
    for (int i = 0; i < n; i++) {
        double w = exp(-((i - m) * (i - m)) / (2.0 * s * s));

        acc  += w * price[end - n + 1 + i];
        wsum += w;
    }
    return acc / wsum;
}

double alma_cross_sw(float* price, int* trade, int n)
{
    double prev_s = 0.0;
    double prev_l = 0.0;
    int have_prev = 0;

    double cash = START_CASH;
    double debt = 0.0;
    int shares = 0;

    for (int i = 0; i < n; i++) {
        trade[i] = 0;

        if (i + 1 < LONG_WINDOW) continue;

        double s = alma(price, i, SHORT_WINDOW);
        double l = alma(price, i, LONG_WINDOW);

        if (have_prev && prev_s <= prev_l && s > l)
            trade[i] = 1;
        else if (have_prev && prev_s >= prev_l && s < l)
            trade[i] = -1;

        prev_s = s;
        prev_l = l;
        have_prev = 1;

        if (trade[i] == 1) {
            if (cash < price[i]) {
                cash += START_CASH;
                debt += START_CASH;
            }

            int bought = (int)(cash / price[i]);
            shares += bought;
            cash -= bought * price[i];
        }
        else if (trade[i] == -1) {
            if (shares == 0) {
                cash += shares * price[i];
                shares = 0;
            }
        }
    }

    return cash + shares * price[n - 1] - debt;
}

int main(void)
{
    int n = NUM_TICKS;
    unsigned seed = SEED;
    srandom(seed);

    float* price    = malloc(n * sizeof *price);
    int*   trade_sw = malloc(n * sizeof *trade_sw);
    int*   trade_hw = malloc(n * sizeof *trade_hw);

    if (!price || !trade_sw || !trade_hw) {
        printf("out of memory\n");
        return -1;
    }

    double p = START_PRICE;
    for (int i = 0; i < n; i++) {
        if (i > 0) p *= 1.0 + random_price_delta();
        price[i] = (float)p;
    }

    double net_sw = alma_cross_sw(price, trade_sw, n);
    alma_cross_hw(price, trade_hw, n);

    int errors = 0;
    int sw_trades = 0;
    int hw_trades = 0;

    for (int i = 0; i < n; i++) {
        sw_trades += trade_sw[i] != 0;
        hw_trades += trade_hw[i] != 0;

        if (trade_sw[i] != trade_hw[i]) {
            if (errors < 20)
                printf("mismach %d: sw=%d hw=%d (price %.2f)\n",
                        i, trade_sw[i], trade_hw[i], price[i]);
            errors++;
        }
    }

    printf("trades: sw=%d hw=%d\n", sw_trades, hw_trades);
    printf("net: sw=%.2f\n", net_sw);
    printf("%s\n", errors ? "TEST FAILED" : "TEST PASSED");

    return 0;
}
