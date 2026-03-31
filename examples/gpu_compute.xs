~~ GPU Compute Example in X#
~~ Demonstrates @gpu blocks for parallel computation

summon Math from "stdlib/math"

~~ Vector addition on GPU
forge vector_add(a: arsenal[spark], b: arsenal[spark]) -> arsenal[spark] {
    morph n: blade = a.length()
    morph result: arsenal[spark] = []
    cycle (morph i: blade = 0; i < n; i++) {
        result.push(0.0)
    }

    @gpu {
        ~~ Each GPU thread handles one element
        ~~ thread_id is implicitly available in @gpu blocks
        morph idx: blade = thread_id
        oracle (idx < n) {
            result[idx] = a[idx] + b[idx]
        }
    }

    unleash result
}

~~ Matrix multiplication on GPU
forge matrix_multiply(a: arsenal[arsenal[spark]], b: arsenal[arsenal[spark]],
                      rows_a: blade, cols_a: blade, cols_b: blade) -> arsenal[arsenal[spark]] {
    morph result: arsenal[arsenal[spark]] = []
    cycle (morph i: blade = 0; i < rows_a; i++) {
        morph row: arsenal[spark] = []
        cycle (morph j: blade = 0; j < cols_b; j++) {
            row.push(0.0)
        }
        result.push(row)
    }

    @gpu {
        ~~ 2D dispatch: thread_x = row, thread_y = column
        morph row: blade = thread_x
        morph col: blade = thread_y
        oracle (row < rows_a && col < cols_b) {
            morph sum: spark = 0.0
            cycle (morph k: blade = 0; k < cols_a; k++) {
                sum = sum + a[row][k] * b[k][col]
            }
            result[row][col] = sum
        }
    }

    unleash result
}

~~ Mandelbrot set computation on GPU
forge compute_mandelbrot(width: blade, height: blade, max_iter: blade) -> arsenal[arsenal[blade]] {
    morph pixels: arsenal[arsenal[blade]] = []
    cycle (morph y: blade = 0; y < height; y++) {
        morph row: arsenal[blade] = []
        cycle (morph x: blade = 0; x < width; x++) {
            row.push(0)
        }
        pixels.push(row)
    }

    @gpu {
        morph px: blade = thread_x
        morph py: blade = thread_y
        oracle (px < width && py < height) {
            ~~ Map pixel to complex plane [-2, 1] x [-1.5, 1.5]
            morph cx: spark = (px as spark / width as spark) * 3.0 - 2.0
            morph cy: spark = (py as spark / height as spark) * 3.0 - 1.5

            morph zx: spark = 0.0
            morph zy: spark = 0.0
            morph iter: blade = 0

            while (zx * zx + zy * zy < 4.0 && iter < max_iter) {
                morph tmp: spark = zx * zx - zy * zy + cx
                zy = 2.0 * zx * zy + cy
                zx = tmp
                iter = iter + 1
            }

            pixels[py][px] = iter
        }
    }

    unleash pixels
}

~~ Parallel reduce: sum all elements
forge parallel_sum(data: arsenal[spark]) -> spark {
    morph n: blade = data.length()
    morph partial: arsenal[spark] = []
    cycle (morph i: blade = 0; i < n; i++) {
        partial.push(data[i])
    }

    ~~ Tree reduction on GPU
    morph stride: blade = 1
    while (stride < n) {
        @gpu {
            morph idx: blade = thread_id * stride * 2
            oracle (idx + stride < n) {
                partial[idx] = partial[idx] + partial[idx + stride]
            }
        }
        stride = stride * 2
    }

    unleash partial[0]
}

~~ Particle simulation step on GPU
entity Particle {
    morph x: spark
    morph y: spark
    morph vx: spark
    morph vy: spark
    morph mass: spark
}

forge simulate_particles(particles: arsenal[Particle], dt: spark, gravity: spark) {
    morph n: blade = particles.length()

    @gpu {
        morph idx: blade = thread_id
        oracle (idx < n) {
            ~~ Apply gravity
            particles[idx].vy = particles[idx].vy + gravity * dt

            ~~ Update position
            particles[idx].x = particles[idx].x + particles[idx].vx * dt
            particles[idx].y = particles[idx].y + particles[idx].vy * dt

            ~~ Simple ground collision at y = 0
            oracle (particles[idx].y < 0.0) {
                particles[idx].y = 0.0
                particles[idx].vy = -particles[idx].vy * 0.8
            }
        }
    }
}

quest() {
    engrave("=== X# GPU Compute Examples ===")
    engrave("")

    ~~ Vector addition
    engrave("--- Vector Addition ---")
    morph a: arsenal[spark] = [1.0, 2.0, 3.0, 4.0, 5.0]
    morph b: arsenal[spark] = [10.0, 20.0, 30.0, 40.0, 50.0]
    morph c: arsenal[spark] = vector_add(a, b)
    engrave("  a + b = [#{c[0]}, #{c[1]}, #{c[2]}, #{c[3]}, #{c[4]}]")
    engrave("")

    ~~ Matrix multiplication
    engrave("--- Matrix Multiplication (2x3 * 3x2) ---")
    morph mat_a: arsenal[arsenal[spark]] = [
        [1.0, 2.0, 3.0],
        [4.0, 5.0, 6.0]
    ]
    morph mat_b: arsenal[arsenal[spark]] = [
        [7.0, 8.0],
        [9.0, 10.0],
        [11.0, 12.0]
    ]
    morph mat_c: arsenal[arsenal[spark]] = matrix_multiply(mat_a, mat_b, 2, 3, 2)
    engrave("  Result:")
    engrave("    [#{mat_c[0][0]}, #{mat_c[0][1]}]")
    engrave("    [#{mat_c[1][0]}, #{mat_c[1][1]}]")
    engrave("")

    ~~ Mandelbrot
    engrave("--- Mandelbrot Set (40x20, ASCII) ---")
    morph mandelbrot: arsenal[arsenal[blade]] = compute_mandelbrot(40, 20, 100)
    morph chars: scroll = " .:-=+*#%@"
    cycle (morph y: blade = 0; y < 20; y++) {
        morph line: scroll = "  "
        cycle (morph x: blade = 0; x < 40; x++) {
            morph idx: blade = mandelbrot[y][x] % 10
            line = line + chars[idx]
        }
        engrave(line)
    }
    engrave("")

    ~~ Parallel sum
    engrave("--- Parallel Reduction (Sum) ---")
    morph data: arsenal[spark] = []
    cycle (morph i: blade = 0; i < 1024; i++) {
        data.push(i as spark)
    }
    morph total: spark = parallel_sum(data)
    engrave("  Sum of 0..1023 = #{total}")
    engrave("  (expected: 523776)")
    engrave("")

    ~~ Particle simulation
    engrave("--- Particle Simulation (5 steps) ---")
    morph particles: arsenal[Particle] = []
    cycle (morph i: blade = 0; i < 4; i++) {
        morph p: Particle = conjure Particle()
        p.x = i as spark * 10.0
        p.y = 100.0
        p.vx = Math.randomRange(-5.0, 5.0)
        p.vy = 0.0
        p.mass = 1.0
        particles.push(p)
    }

    cycle (morph step: blade = 0; step < 5; step++) {
        simulate_particles(particles, 0.1, -9.81)
        engrave("  Step #{step}:")
        cycle (morph i: blade = 0; i < particles.length(); i++) {
            engrave("    P#{i}: (#{particles[i].x}, #{particles[i].y})")
        }
    }

    engrave("")
    engrave("GPU compute demo complete!")
}
