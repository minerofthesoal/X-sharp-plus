~~ Fibonacci Example in X#
~~ Demonstrates recursive and iterative implementations

~~ Recursive Fibonacci
forge fib_recursive(n: blade) -> blade {
    oracle (n <= 0) { unleash 0 }
    oracle (n == 1) { unleash 1 }
    unleash fib_recursive(n - 1) + fib_recursive(n - 2)
}

~~ Iterative Fibonacci
forge fib_iterative(n: blade) -> blade {
    oracle (n <= 0) { unleash 0 }
    oracle (n == 1) { unleash 1 }

    morph prev: blade = 0
    morph curr: blade = 1

    cycle (morph i: blade = 2; i <= n; i++) {
        morph next: blade = prev + curr
        prev = curr
        curr = next
    }

    unleash curr
}

~~ Memoized Fibonacci using an arsenal (array)
forge fib_memoized(n: blade) -> blade {
    morph memo: arsenal[blade] = []
    ~~ Initialize memo table with -1
    cycle (morph i: blade = 0; i <= n; i++) {
        memo.push(-1)
    }
    memo[0] = 0
    oracle (n >= 1) { memo[1] = 1 }

    unleash fib_memo_helper(n, memo)
}

forge fib_memo_helper(n: blade, memo: arsenal[blade]) -> blade {
    oracle (memo[n] != -1) { unleash memo[n] }
    memo[n] = fib_memo_helper(n - 1, memo) + fib_memo_helper(n - 2, memo)
    unleash memo[n]
}

~~ Generator-style: print Fibonacci sequence up to limit
forge print_fib_sequence(limit: blade) {
    morph a: blade = 0
    morph b: blade = 1
    morph count: blade = 0

    while (a <= limit) {
        engrave("  F(#{count}) = #{a}")
        morph temp: blade = a + b
        a = b
        b = temp
        count = count + 1
    }
}

~~ Golden ratio approximation using consecutive Fibonacci numbers
forge golden_ratio_approx(terms: blade) -> spark {
    morph a: spark = 1.0
    morph b: spark = 1.0
    cycle (morph i: blade = 0; i < terms; i++) {
        morph temp: spark = a + b
        a = b
        b = temp
    }
    unleash b / a
}

~~ Main entry point
quest() {
    engrave("=== X# Fibonacci Examples ===")
    engrave("")

    ~~ Recursive version (small n due to exponential time)
    engrave("Recursive Fibonacci (n=0..15):")
    cycle (morph i: blade = 0; i <= 15; i++) {
        morph result: blade = fib_recursive(i)
        engrave("  fib_recursive(#{i}) = #{result}")
    }
    engrave("")

    ~~ Iterative version (handles larger n)
    engrave("Iterative Fibonacci (selected values):")
    morph test_values: arsenal[blade] = [0, 1, 5, 10, 20, 30, 40, 50]
    cycle (morph i: blade = 0; i < test_values.length(); i++) {
        morph n: blade = test_values[i]
        morph result: blade = fib_iterative(n)
        engrave("  fib_iterative(#{n}) = #{result}")
    }
    engrave("")

    ~~ Memoized version
    engrave("Memoized Fibonacci (n=25):")
    morph memo_result: blade = fib_memoized(25)
    engrave("  fib_memoized(25) = #{memo_result}")
    engrave("")

    ~~ Fibonacci sequence
    engrave("Fibonacci sequence up to 1000:")
    print_fib_sequence(1000)
    engrave("")

    ~~ Golden ratio approximation
    engrave("Golden ratio approximation:")
    morph phi: spark = golden_ratio_approx(50)
    engrave("  phi ~= #{phi}")
    engrave("  (actual: 1.6180339887...)")
}
