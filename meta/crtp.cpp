#include <print>
#include <type_traits>

// CRTP，全称 Curiously Recurring Template Pattern，是一种经典的模板惯用法

// I. 静态多态

template <typename Derived> struct Animal {
  void makeSound() { static_cast<Derived *>(this)->makeSoundImpl(); }
  // char buffer[sizeof(Derived)];
  // invalid application of 'sizeof' to an incomplete type 'Dog'
};

struct Dog : Animal<Dog> {
  void makeSoundImpl() { std::println("Woof!"); }
};

struct Cat : Animal<Cat> {
  void makeSoundImpl() { std::println("Meow!"); }
};

template <typename T> void playWithAnimal(Animal<T> &animal) {
  animal.makeSound();
}

// 没有了查找虚表的开销，并且编译器可以进行内联优化，在内存布局和性能上都比传统的虚表多态好
// 一开始看可能不是很好理解，比如 struct Dog : Animal<Dog>
// 看起来就像一个自指，没办法实例化
// 不妨这么理解
// 声明 Dog struct Dog;
// 对 Animal 模板实例化，Animal<Dog>
// 此时 Animal<Dog> 的内存布局确定下来，Dog 的定义也随即完成

// II. Mixin class

template <typename T> class ObjectCounter {
protected:
  inline static int count = 0;

public:
  ObjectCounter() { ++count; }
  ObjectCounter(const ObjectCounter &) { ++count; }
  ~ObjectCounter() { --count; }
  static int getCount() { return count; }
};

class Widget : public ObjectCounter<Widget> {
  // ...
};

class Button : public ObjectCounter<Button> {
  // ...
};

int main() {
  Dog dog;
  Cat cat;

  playWithAnimal(dog);
  playWithAnimal(cat);

  Widget w1, w2;
  Button b1;

  std::println("Widgets: {}", Widget::getCount());
  std::println("Buttons: {}", Button::getCount());

  return 0;
}