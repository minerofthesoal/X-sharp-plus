~~ Physics Simulation Demo in X#
~~ Simple physics with gravity, collision, and bouncing

summon Math from "stdlib/math"

~~ 2D Vector helper entity
entity Vec2 {
    morph x: spark
    morph y: spark

    forge conjure(x: spark, y: spark) {
        self.x = x
        self.y = y
    }

    forge add(other: Vec2) -> Vec2 {
        unleash conjure Vec2(self.x + other.x, self.y + other.y)
    }

    forge sub(other: Vec2) -> Vec2 {
        unleash conjure Vec2(self.x - other.x, self.y - other.y)
    }

    forge scale(s: spark) -> Vec2 {
        unleash conjure Vec2(self.x * s, self.y * s)
    }

    forge length() -> spark {
        unleash Math.sqrt(self.x * self.x + self.y * self.y)
    }

    forge normalize() -> Vec2 {
        morph len: spark = self.length()
        oracle (len < 0.0001) { unleash conjure Vec2(0.0, 0.0) }
        unleash conjure Vec2(self.x / len, self.y / len)
    }

    forge dot(other: Vec2) -> spark {
        unleash self.x * other.x + self.y * other.y
    }

    forge to_string() -> scroll {
        unleash "(#{self.x}, #{self.y})"
    }
}

~~ Physics body entity
entity Body {
    morph pos: Vec2
    morph vel: Vec2
    morph acc: Vec2
    morph mass: spark
    morph radius: spark
    morph restitution: spark  ~~ bounciness (0 to 1)
    morph name: scroll

    forge conjure(name: scroll, x: spark, y: spark, mass: spark, radius: spark) {
        self.name = name
        self.pos = conjure Vec2(x, y)
        self.vel = conjure Vec2(0.0, 0.0)
        self.acc = conjure Vec2(0.0, 0.0)
        self.mass = mass
        self.radius = radius
        self.restitution = 0.7
    }

    forge apply_force(force: Vec2) {
        ~~ F = ma, so a = F/m
        morph a: Vec2 = force.scale(1.0 / self.mass)
        self.acc = self.acc.add(a)
    }

    forge update(dt: spark) {
        ~~ Semi-implicit Euler integration
        self.vel = self.vel.add(self.acc.scale(dt))
        self.pos = self.pos.add(self.vel.scale(dt))

        ~~ Reset acceleration
        self.acc = conjure Vec2(0.0, 0.0)
    }

    forge kinetic_energy() -> spark {
        morph speed: spark = self.vel.length()
        unleash 0.5 * self.mass * speed * speed
    }

    forge momentum() -> Vec2 {
        unleash self.vel.scale(self.mass)
    }
}

~~ World entity: manages bodies and physics
entity World {
    morph bodies: arsenal[Body]
    morph gravity: Vec2
    morph floor_y: spark
    morph left_wall: spark
    morph right_wall: spark
    morph ceiling_y: spark
    morph time: spark

    forge conjure(gravity_y: spark) {
        self.bodies = []
        self.gravity = conjure Vec2(0.0, gravity_y)
        self.floor_y = 0.0
        self.ceiling_y = 200.0
        self.left_wall = 0.0
        self.right_wall = 300.0
        self.time = 0.0
    }

    forge add_body(body: Body) {
        self.bodies.push(body)
    }

    forge step(dt: spark) {
        ~~ Apply gravity to all bodies
        cycle (morph i: blade = 0; i < self.bodies.length(); i++) {
            morph gforce: Vec2 = self.gravity.scale(self.bodies[i].mass)
            self.bodies[i].apply_force(gforce)
        }

        ~~ Update positions
        cycle (morph i: blade = 0; i < self.bodies.length(); i++) {
            self.bodies[i].update(dt)
        }

        ~~ Check body-body collisions
        cycle (morph i: blade = 0; i < self.bodies.length(); i++) {
            cycle (morph j: blade = i + 1; j < self.bodies.length(); j++) {
                self.resolve_collision(self.bodies[i], self.bodies[j])
            }
        }

        ~~ Check wall collisions
        cycle (morph i: blade = 0; i < self.bodies.length(); i++) {
            self.resolve_walls(self.bodies[i])
        }

        self.time = self.time + dt
    }

    forge resolve_collision(a: Body, b: Body) {
        morph diff: Vec2 = b.pos.sub(a.pos)
        morph dist: spark = diff.length()
        morph min_dist: spark = a.radius + b.radius

        oracle (dist >= min_dist || dist < 0.0001) { unleash }

        ~~ Collision normal
        morph normal: Vec2 = diff.normalize()

        ~~ Separate overlapping bodies
        morph overlap: spark = min_dist - dist
        morph total_mass: spark = a.mass + b.mass
        a.pos = a.pos.sub(normal.scale(overlap * b.mass / total_mass))
        b.pos = b.pos.add(normal.scale(overlap * a.mass / total_mass))

        ~~ Relative velocity
        morph rel_vel: Vec2 = b.vel.sub(a.vel)
        morph vel_along_normal: spark = rel_vel.dot(normal)

        ~~ Only resolve if bodies are approaching
        oracle (vel_along_normal > 0.0) { unleash }

        ~~ Coefficient of restitution (use minimum)
        morph e: spark = Math.min(a.restitution, b.restitution)

        ~~ Impulse magnitude
        morph j: spark = -(1.0 + e) * vel_along_normal
        j = j / (1.0 / a.mass + 1.0 / b.mass)

        ~~ Apply impulse
        morph impulse: Vec2 = normal.scale(j)
        a.vel = a.vel.sub(impulse.scale(1.0 / a.mass))
        b.vel = b.vel.add(impulse.scale(1.0 / b.mass))
    }

    forge resolve_walls(body: Body) {
        ~~ Floor
        oracle (body.pos.y - body.radius < self.floor_y) {
            body.pos.y = self.floor_y + body.radius
            body.vel.y = -body.vel.y * body.restitution
            ~~ Apply friction
            body.vel.x = body.vel.x * 0.98
        }

        ~~ Ceiling
        oracle (body.pos.y + body.radius > self.ceiling_y) {
            body.pos.y = self.ceiling_y - body.radius
            body.vel.y = -body.vel.y * body.restitution
        }

        ~~ Left wall
        oracle (body.pos.x - body.radius < self.left_wall) {
            body.pos.x = self.left_wall + body.radius
            body.vel.x = -body.vel.x * body.restitution
        }

        ~~ Right wall
        oracle (body.pos.x + body.radius > self.right_wall) {
            body.pos.x = self.right_wall - body.radius
            body.vel.x = -body.vel.x * body.restitution
        }
    }

    forge total_energy() -> spark {
        morph total: spark = 0.0
        cycle (morph i: blade = 0; i < self.bodies.length(); i++) {
            ~~ Kinetic energy
            total = total + self.bodies[i].kinetic_energy()
            ~~ Potential energy (relative to floor)
            morph h: spark = self.bodies[i].pos.y - self.floor_y
            total = total + self.bodies[i].mass * (-self.gravity.y) * h
        }
        unleash total
    }

    forge print_state() {
        engrave("  t = #{self.time}")
        cycle (morph i: blade = 0; i < self.bodies.length(); i++) {
            morph b: Body = self.bodies[i]
            engrave("    #{b.name}: pos=#{b.pos.to_string()} vel=#{b.vel.to_string()} KE=#{b.kinetic_energy()}")
        }
        engrave("    Total energy: #{self.total_energy()}")
    }
}

quest() {
    engrave("=== X# Physics Simulation Demo ===")
    engrave("")

    ~~ Create world with gravity
    morph world: World = conjure World(-9.81)

    ~~ Create bodies
    morph ball1: Body = conjure Body("Ball-A", 50.0, 150.0, 2.0, 10.0)
    ball1.vel = conjure Vec2(30.0, 0.0)

    morph ball2: Body = conjure Body("Ball-B", 200.0, 100.0, 3.0, 15.0)
    ball2.vel = conjure Vec2(-10.0, 5.0)

    morph ball3: Body = conjure Body("Ball-C", 150.0, 180.0, 1.0, 8.0)
    ball3.restitution = 0.9

    world.add_body(ball1)
    world.add_body(ball2)
    world.add_body(ball3)

    engrave("Initial state:")
    world.print_state()
    engrave("")

    ~~ Simulate for 50 steps at 0.02s each (1 second total)
    morph dt: spark = 0.02
    morph total_steps: blade = 50

    cycle (morph step: blade = 0; step < total_steps; step++) {
        world.step(dt)

        ~~ Print every 10 steps
        oracle (step % 10 == 9) {
            engrave("After #{step + 1} steps:")
            world.print_state()
            engrave("")
        }
    }

    engrave("Simulation complete!")
    engrave("Final total energy: #{world.total_energy()}")
}
