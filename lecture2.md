## `decltype` и `decltype(auto)`

Помимо того, что иногда нам надо уметь делать "идеальную передачу" аргументов в некоторую функцию, порой нам хочется
делать "идеальное возвращение" значения, которое возвращает функция (например, если она возвращает значение, то вернём
значение, а если возвращает по ссылке, то и вернём мы из нашей обёртки тоже ссылку)

```c++
template <typename F, typename ...Ts>
/*непонятно, что тут написать*/ logged(F f, Ts &&...args) {
    std::cout << "Before function invocation" << std::endl;
    return f((std::forward<Ts>(args))...);
}
```

Можно вспомнить про `auto`, но, к сожалению, он тут не поможет:

> Type deduction drops references

> Type deduction drops const qualifiers

Чтобы реализовать идеальное возвращение значения из функции, нужно понять, как работает `decltype`, который мы уже в
какой-то момент встречали (например, когда разговаривали про функторы и лямды)

```c++
int x = 10;
int &y = x;

decltype(10); // int
decltype(10 + 20); // int
decltype(x); // int
decltype(y); // int &
decltype(y + 20); // int
decltype((x)); // int &
```

Особое удивление наверняка вызывает `decltype((x))`, однако это вполне объяснимо, учитывая как работает `decltype`

### На самом деле, у него есть 2 режима работы:

1. `decltype(expression)` (вызываем `decltype` от выражения)
    1. `decltype(10)`
    2. `decltype((x))`
    3. `decltype(x + 20)`
2. `decltype(variable)` (вызываем `decltype` от какой-то переменной)
    1. `decltype(x)`

**В первом случае** он работает следующим образом:

1. Если то, что возвращает выражение -- это `prvalue`, то `decltype` нам вернёт сам тип `T`

2. Если то, что возвращает выражение -- это `lvalue`, то `decltype` нам вернёт `T&`

3. Если то, что возвращает выражение -- это `xvalue`, то `decltype` нам вернёт `T&&`

По сути, отдельно считаем тип и категорию выражения и из этого собираем тип, который возвращает

**Во втором случае** он работает очень просто: смотрит, с каким типом переменная была объявлена

Вот небольшие примеры того, как `auto` и `decltype(auto)` работают в выводе возвращаемого значения функции

```c++
int x;

auto foo() {
    // --> int foo()
    return x;
}

decltype(auto) bar() {
    // --> int bar (выводится int, так как 'x' был объявлен c типом int)
    return x;
}

decltype(auto) baz() {
    // --> int & baz() (выводится int&, так как тип выводится по правилам decltype)
    // это выражение
    return (x);
}

```

Напишем, как теперь выглядит идеальное возвращение значения со знанием правил работы `decltype`

```c++
template <typename F, typename ...Ts>
decltype(auto) logged(F f, Ts &&...args) {
  std::cout << "Before function invocation" << std::endl;
  return f((std::forward<Ts>(args))...);
}
```

А ещё всё это можно написать в виде лямбды

```c++
auto logged = [&](auto f, auto &&...args) -> decltype(auto) {
  std::cout << "Before function invocation" << std::endl;
  return f(std::forward<decltype(args)>(args)...);
};
```

