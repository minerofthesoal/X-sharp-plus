~~ Entity (Class) System Demo in X#
~~ Demonstrates entities, inheritance, methods, constructors

~~ Base entity: Shape
entity Shape {
    morph name: scroll
    morph color: scroll = "white"

    forge conjure(name: scroll) {
        self.name = name
    }

    forge describe() -> scroll {
        unleash "Shape: #{self.name} (#{self.color})"
    }

    forge area() -> spark {
        unleash 0.0
    }

    forge perimeter() -> spark {
        unleash 0.0
    }
}

~~ Circle extends Shape
entity Circle extends Shape {
    morph radius: spark

    forge conjure(radius: spark) {
        self.name = "Circle"
        self.radius = radius
    }

    forge area() -> spark {
        unleash 3.14159265 * self.radius * self.radius
    }

    forge perimeter() -> spark {
        unleash 2.0 * 3.14159265 * self.radius
    }

    forge describe() -> scroll {
        unleash "Circle(r=#{self.radius}, color=#{self.color})"
    }
}

~~ Rectangle extends Shape
entity Rectangle extends Shape {
    morph width: spark
    morph height: spark

    forge conjure(width: spark, height: spark) {
        self.name = "Rectangle"
        self.width = width
        self.height = height
    }

    forge area() -> spark {
        unleash self.width * self.height
    }

    forge perimeter() -> spark {
        unleash 2.0 * (self.width + self.height)
    }

    forge describe() -> scroll {
        unleash "Rectangle(#{self.width}x#{self.height}, color=#{self.color})"
    }
}

~~ Square extends Rectangle
entity Square extends Rectangle {
    forge conjure(side: spark) {
        self.name = "Square"
        self.width = side
        self.height = side
    }

    forge describe() -> scroll {
        unleash "Square(side=#{self.width}, color=#{self.color})"
    }
}

~~ Triangle extends Shape
entity Triangle extends Shape {
    morph a: spark
    morph b: spark
    morph c: spark

    forge conjure(a: spark, b: spark, c: spark) {
        self.name = "Triangle"
        self.a = a
        self.b = b
        self.c = c
    }

    forge perimeter() -> spark {
        unleash self.a + self.b + self.c
    }

    forge area() -> spark {
        ~~ Heron's formula
        morph s: spark = self.perimeter() / 2.0
        morph val: spark = s * (s - self.a) * (s - self.b) * (s - self.c)
        ~~ Approximate sqrt using Newton's method
        morph x: spark = val / 2.0
        cycle (morph i: blade = 0; i < 20; i++) {
            x = (x + val / x) / 2.0
        }
        unleash x
    }

    forge describe() -> scroll {
        unleash "Triangle(#{self.a}, #{self.b}, #{self.c}, color=#{self.color})"
    }
}

~~ Animal hierarchy to show more inheritance patterns
entity Animal {
    morph species: scroll
    morph legs: blade
    morph sound: scroll = "..."

    forge conjure(species: scroll, legs: blade) {
        self.species = species
        self.legs = legs
    }

    forge speak() -> scroll {
        unleash "#{self.species} says #{self.sound}"
    }

    forge info() -> scroll {
        unleash "#{self.species}: #{self.legs} legs"
    }
}

entity Dog extends Animal {
    morph breed: scroll

    forge conjure(breed: scroll) {
        self.species = "Dog"
        self.legs = 4
        self.sound = "Woof!"
        self.breed = breed
    }

    forge fetch(item: scroll) -> scroll {
        unleash "#{self.breed} fetches the #{item}!"
    }
}

entity Cat extends Animal {
    morph indoor: fate

    forge conjure(indoor: fate) {
        self.species = "Cat"
        self.legs = 4
        self.sound = "Meow!"
        self.indoor = indoor
    }

    forge purr() -> scroll {
        unleash "#{self.species} purrs contentedly..."
    }
}

entity Bird extends Animal {
    morph can_fly: fate

    forge conjure(can_fly: fate) {
        self.species = "Bird"
        self.legs = 2
        self.sound = "Tweet!"
        self.can_fly = can_fly
    }

    forge fly() -> scroll {
        oracle (self.can_fly) {
            unleash "#{self.species} soars through the sky!"
        } otherwise {
            unleash "#{self.species} flaps but stays on the ground."
        }
    }
}

~~ Utility: print a shape's full info
forge print_shape_info(shape: Shape) {
    engrave("  #{shape.describe()}")
    engrave("    Area:      #{shape.area()}")
    engrave("    Perimeter: #{shape.perimeter()}")
}

~~ Main entry point
quest() {
    engrave("=== X# Entity (Class) System Demo ===")
    engrave("")

    ~~ Shape hierarchy
    engrave("--- Shapes ---")
    morph circle: Circle = conjure Circle(5.0)
    circle.color = "red"

    morph rect: Rectangle = conjure Rectangle(4.0, 7.0)
    rect.color = "blue"

    morph square: Square = conjure Square(3.0)
    square.color = "green"

    morph tri: Triangle = conjure Triangle(3.0, 4.0, 5.0)
    tri.color = "yellow"

    morph shapes: arsenal[Shape] = [circle, rect, square, tri]
    cycle (morph i: blade = 0; i < shapes.length(); i++) {
        print_shape_info(shapes[i])
    }

    engrave("")

    ~~ Animal hierarchy
    engrave("--- Animals ---")
    morph dog: Dog = conjure Dog("Golden Retriever")
    morph cat: Cat = conjure Cat(truth)
    morph eagle: Bird = conjure Bird(truth)
    morph penguin: Bird = conjure Bird(lies)

    morph animals: arsenal[Animal] = [dog, cat, eagle, penguin]
    cycle (morph i: blade = 0; i < animals.length(); i++) {
        engrave("  #{animals[i].info()}")
        engrave("    #{animals[i].speak()}")
    }

    engrave("")
    engrave("  #{dog.fetch("ball")}")
    engrave("  #{cat.purr()}")
    engrave("  #{eagle.fly()}")
    engrave("  #{penguin.fly()}")

    engrave("")
    engrave("Entity demo complete!")
}
