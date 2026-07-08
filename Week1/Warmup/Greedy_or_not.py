import sys


def solve(nums):
    """
    Two players alternately take from either end of `nums`, each maximizing
    their own total. Returns dp[0][n-1]: the optimal score difference
    (current mover's total - opponent's total) achievable from the whole list.

    dp[i] is kept rolling across increasing subarray length L, so that after
    the inner loop for length L, dp[i] holds dp[i][i+L-1]. This works in a
    single pass because, while filling dp[i] for the new length, dp[i+1] has
    not been touched yet this round and still holds the previous length's
    value dp[i+1][j], which is exactly the term the recurrence needs.
    """
    n = len(nums)
    dp = nums[:]
    for length in range(2, n + 1):
        for i in range(n - length + 1):
            j = i + length - 1
            dp[i] = max(nums[i] - dp[i + 1], nums[j] - dp[i])
    return dp[0]


def main():
    data = sys.stdin.read().split()
    n = int(data[0])
    nums = list(map(int, data[1:1 + n]))

    diff = solve(nums)
    if diff > 0:
        print("Player 1 wins")
    elif diff < 0:
        print("Player 2 wins")
    else:
        print("Its a draw")


if __name__ == "__main__":
    main()
