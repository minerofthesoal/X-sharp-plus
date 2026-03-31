~~ Game Loop Example in X#
~~ Creates a window, draws shapes, handles input, runs at 60fps

summon Renderer from "engine/renderer"
summon Input from "engine/input"
summon Math from "stdlib/math"

~~ Game state
morph player_x: spark = 400.0
morph player_y: spark = 300.0
morph player_speed: spark = 200.0
morph player_angle: spark = 0.0
morph running: fate = truth

~~ Enemy entity
entity Enemy {
    morph x: spark
    morph y: spark
    morph radius: spark = 15.0
    morph alive: fate = truth
    morph speed: spark = 50.0
    morph angle: spark = 0.0

    forge conjure(start_x: spark, start_y: spark) {
        self.x = start_x
        self.y = start_y
        self.angle = Math.random() * Math.TAU()
    }

    forge update(dt: spark) {
        oracle (!self.alive) { unleash }
        self.x = self.x + Math.cos(self.angle) * self.speed * dt
        self.y = self.y + Math.sin(self.angle) * self.speed * dt

        ~~ Bounce off screen edges
        oracle (self.x < 0 || self.x > 800.0) {
            self.angle = Math.PI() - self.angle
        }
        oracle (self.y < 0 || self.y > 600.0) {
            self.angle = -self.angle
        }

        self.x = Math.clamp(self.x, 0.0, 800.0)
        self.y = Math.clamp(self.y, 0.0, 600.0)
    }

    forge draw(renderer: Renderer) {
        oracle (self.alive) {
            renderer.fill_circle(self.x, self.y, self.radius, 0xFF0000FF)
        }
    }
}

~~ Star background particle
entity Star {
    morph x: spark
    morph y: spark
    morph brightness: spark

    forge conjure() {
        self.x = Math.randomRange(0.0, 800.0)
        self.y = Math.randomRange(0.0, 600.0)
        self.brightness = Math.randomRange(0.3, 1.0)
    }

    forge draw(renderer: Renderer) {
        morph alpha: blade = (self.brightness * 255.0) as blade
        renderer.set_pixel(self.x as blade, self.y as blade, 0xFFFFFF00 | alpha)
    }
}

~~ Initialize enemies
morph enemies: arsenal[Enemy] = []
cycle (morph i: blade = 0; i < 10; i++) {
    morph ex: spark = Math.randomRange(50.0, 750.0)
    morph ey: spark = Math.randomRange(50.0, 550.0)
    enemies.push(conjure Enemy(ex, ey))
}

~~ Initialize stars
morph stars: arsenal[Star] = []
cycle (morph i: blade = 0; i < 100; i++) {
    stars.push(conjure Star())
}

~~ Score tracking
morph score: blade = 0
morph frame_count: blade = 0

forge update(dt: spark) {
    ~~ Handle input
    oracle (Input.is_key_down("W") || Input.is_key_down("Up")) {
        player_y = player_y - player_speed * dt
    }
    oracle (Input.is_key_down("S") || Input.is_key_down("Down")) {
        player_y = player_y + player_speed * dt
    }
    oracle (Input.is_key_down("A") || Input.is_key_down("Left")) {
        player_x = player_x - player_speed * dt
    }
    oracle (Input.is_key_down("D") || Input.is_key_down("Right")) {
        player_x = player_x + player_speed * dt
    }
    oracle (Input.is_key_down("Escape")) {
        running = lies
    }

    ~~ Clamp player position
    player_x = Math.clamp(player_x, 20.0, 780.0)
    player_y = Math.clamp(player_y, 20.0, 580.0)

    ~~ Rotate player toward mouse
    morph mx: spark = Input.mouse_x() as spark
    morph my: spark = Input.mouse_y() as spark
    player_angle = Math.atan2(my - player_y, mx - player_x)

    ~~ Update enemies
    cycle (morph i: blade = 0; i < enemies.length(); i++) {
        enemies[i].update(dt)

        ~~ Check collision with player
        morph dx: spark = enemies[i].x - player_x
        morph dy: spark = enemies[i].y - player_y
        morph dist: spark = Math.sqrt(dx * dx + dy * dy)
        oracle (dist < 20.0 + enemies[i].radius && enemies[i].alive) {
            enemies[i].alive = lies
            score = score + 100
        }
    }

    frame_count = frame_count + 1
}

forge draw(renderer: Renderer) {
    ~~ Clear screen to dark blue
    renderer.clear(0x001122FF)

    ~~ Draw stars
    cycle (morph i: blade = 0; i < stars.length(); i++) {
        stars[i].draw(renderer)
    }

    ~~ Draw enemies
    cycle (morph i: blade = 0; i < enemies.length(); i++) {
        enemies[i].draw(renderer)
    }

    ~~ Draw player (triangle pointing in movement direction)
    morph cos_a: spark = Math.cos(player_angle)
    morph sin_a: spark = Math.sin(player_angle)
    morph tip_x: spark = player_x + cos_a * 20.0
    morph tip_y: spark = player_y + sin_a * 20.0
    morph left_x: spark = player_x + Math.cos(player_angle + 2.5) * 12.0
    morph left_y: spark = player_y + Math.sin(player_angle + 2.5) * 12.0
    morph right_x: spark = player_x + Math.cos(player_angle - 2.5) * 12.0
    morph right_y: spark = player_y + Math.sin(player_angle - 2.5) * 12.0

    renderer.fill_triangle(
        tip_x, tip_y,
        left_x, left_y,
        right_x, right_y,
        0x00FF88FF
    )

    ~~ Draw score
    renderer.draw_text(10, 10, "Score: #{score}", 0xFFFFFFFF)

    ~~ Draw FPS
    renderer.draw_text(700, 10, "FPS: 60", 0xCCCCCCFF)
}

~~ Main entry point
quest() {
    morph renderer: Renderer = conjure Renderer(800, 600, "X# Game Demo")
    morph target_dt: spark = 1.0 / 60.0

    engrave("Game started! Use WASD to move, ESC to quit.")
    engrave("Collide with red enemies to score points!")

    while (running) {
        Input.poll()
        update(target_dt)
        draw(renderer)
        renderer.present()
        renderer.wait_frame(target_dt)
    }

    engrave("Final score: #{score}")
    engrave("Game over!")
}
