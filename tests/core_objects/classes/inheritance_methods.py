class Animal:
    kind = "animal"

    def __init__(self, name):
        self.name = name

    def speak(self):
        return self.name + " makes a sound"


class Dog(Animal):
    kind = "dog"

    def speak(self):
        return self.name + " barks"

    def describe(self):
        return super().speak() + "; " + self.speak()


pet = Dog("Rex")
assert isinstance(pet, Dog)
assert isinstance(pet, Animal)
assert pet.name == "Rex"
assert pet.kind == "dog"
assert Animal.kind == "animal"
assert pet.speak() == "Rex barks"
assert pet.describe() == "Rex makes a sound; Rex barks"
assert Dog.speak(pet) == "Rex barks"
assert Animal.speak(pet) == "Rex makes a sound"

print("inheritance_methods: PASS")
