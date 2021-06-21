# Исключения

## Общая информация

Ошибки бывают разные, исходя из некоторых нужно уметь делать что-то разумное (не открылся документ - вывести сообщения и т.д.) 
\- это recoverable error. Другой вид ошибок \- это баги (bug) \- нарушился какой-то инвариант, все очень плохо. 
При баге лучше быстро упасть, возможно что-то сказав программисту. Пользователя не спасти.

Можно почитать статью: http://joeduffyblog.com/2016/02/07/the-error-model/

## Разные техники обработки ошибок

### exit, assert

Лучше завершить полностью всю программу если произошло что-то явно некорректное.

```c++
void read_people() {
    FILE *f = fopen(filename "r");
    if (!f) {  // Не удалось открыть файл — вернули nullptr. Ошибка окружения. Что делаем?
        printf("Unable to open file\n");
        // return;	// Если сделаем просто return, то непонятно что будет происходить дальше.
        exit(1);  // Поэтому правильным решением будет завершить всю программу.
    }
	// assert(f) // Можно было проверить так, но была бы проблема: assert могут вырезаться в Release режиме компиляции.
    // ...
    fclose(f);
}
```

### Возвращать код ошибки

Можно хранить глобальную переменную *errno*, в которую записывать ошибки.

```c++
#define EACCES
#define EBADF
// ....
int main() {
    // ....
    if (chdir("hello-dir") != 0) {
        // А что если несколько функций подряд?
        if (errno == EACCES) { /* .... } 
		else if (errno == EBADF) { /* .... /* } 
		else {/* .... */}
    }
}
```

С этим возникают некоторые проблемы, например, теперь всегда нужно писать *if (func)*, *errno* могла в процессе измениться 
в разных функциях и может быть сложно понять как конкретно обработать ошибку и т.д. Обычно если в функции произошла ошибка, то она возвращает 0.
У функции есть свои коды для разных ошибок, а в возвращаемую структуру можно положить значение, которое явно будет указывать на некорректный результат 
(например, *0* в *int*, *NULL* в *\*int*). Это выглядить как-то так:

```c++
if ((err = sqlite3_open("hello_sqlite3", &db)) != 0) { /* Здесь идет орбаботка ошибок. db = nullptr/Null */}
```

Идеалогия *Go, Rust, Haskell*: возвращать результирующую структуру или строку с ошибкой, если что-то пошло не так.

```Rust
fn read_people() -> Result<People, String> { /* .... */}
```

## Исключения: try/catch/throw, slicing, automatic storage duration

### Основной синтаксис.

Если возникла ошибка, которую мы можем обработать, то можно кинуть исключение: *throw smth_struct* (также поддерживается *throw int*, 
но не очень понятно зачем это нужно), которое потом нужно поймать в блоке try и обработать в нужном блоке catch, 
который ловит соответсвующую структуру.

```c++
struct invalid_vector_format {};

std::vector<int> readVector() {
    int n;
    if (!(std::cin >> n)) {
        throw invalid_vector_format();
    }
    std::vector<int> result(n); // может кинуть исключение.
    for (int i = 0; i < n; i++) {
        if (!(std::cin >> result[i])) {
            throw invalid_vector_format(); // кинул исключение, знаем, что кто-нибудь поймает.
        }
    }
    return result;
}

void solve() {
    std::vector<int> a = readVector();  // исключение пролетело насквозь
	// ....
}

int main() {
    try {
        solve(); // Отсюда вылетают исключения
    } catch (const invalid_vector_format &err) { // А здесь они ловятся
        std::cout << "Invalid vector format\n";
    }
}
```

Если где-нибудь возникает *throw*, то компилятор начинает выполнять раскрутку стека (*automatic storage duration*): начинает идти вверх по стеку, 
т.е. делает return из функций по очереди, пока не встретит *try/catch*, далее ищет соответствующий *catch* и выполняет его, если находит.
Если вылетело исключение, то код дальше не исполняется. В данном случае *vector<int> a* даже не инициализируется.

Можно разные ошибки обрабатывать по отдельности:

```c++
struct err1 {};
struct err2 {};

int main() {
    try { std::vector<int> vec(-1); } 
	catch (const err1 &) { std::cout << "1\n"; } 
	catch (const err2 &) { std::cout << "2\n"; } 
	catch (...) { // Специальный случай: если среди обработанных исключений нет текущего, то заходим сюда
        std::cout << "3\n";
    }
}
```

### Еще про раскрутку стека

Нужно понимать, что в момент выкидывания исключения через *throw*, все созданные объекты на текущем уровне сразу удаляются 
(у них корректно вызываются деструкторы) и код дальше не выполняется. Исключения также можно кидать из блока *try/catch*

- *catch* будет обрабатывать все исключения, выкинутые из блока *try*;
- *catch* не обрабатывает исключения, выкинутые из другого *catch* на текущем уровне
- Необработанное усключение летит по уровням вверх, уничтожая все объекты на каждом уровне, пока не попадет в *try* или в *main* 
(если попадет в *main*, то программа завершится с данной ошибкой).

Большой пример:

```c++
struct err1 {};
struct err2 {};

void bar() {
    try {
        std::cout << "3\n";
        throw err1();         // Выкидывает
        std::cout << "x3\n";
    } catch (const err1 &e) { // Ловит
        std::cout << "4\n";
        throw err2();         // Тоже выкидывает
        std::cout << "x4\n";
    } catch (const err2 &e) { // Уже не ловит
        std::cout << "x5\n";
    }
    std::cout << "bar end\n";
}                             // Вывод: "3\n4\n"

void foo() {
    try {
        try {
            std::cout << "2\n";
            bar();            // Отсюда вылетает err2 
			throw err2();     // До этой строчки не доходит
            std::cout << "x2\n";
        } catch (const err1 &e) { // Не та структура
            std::cout << "z\n";
        }
        std::cout << "x21\n";
    } catch (int e) {         // err2 != int
        std::cout << "x22\n";
    }
    std::cout << "foo end\n";
}                             // Вывод: "2\n3\n\4\n"

int main() {
    try {
        std::cout << "1\n";
        foo();                // Отсюда вылетает err2
        std::cout << "xxx\n";
    } catch (const err2 &) {  // Поймали
        std::cout << "5\n";
    }
    std::cout << "main continues\n";
}                             //Итого Вывод: "1\n\2\n3\n4\n5\nmain continues\n"
```

Есть спецальный синтаксис: переброска исключения:

```c++
int foo(){
    try{ 
	    throw 1; 
	}
	catch (const err1&){ /* smth code */ }
	catch (const err2&){ /* smth code */ }
	catch (...){
	    throw; // Перебросили текущее исключение
	}
}
```

### Вызов деструкторов при throw

Деструкторы не всегда вызываются у объектов: если компилятор видит, что программа все равно завершится, то он может вырезать вызов деструкторов.
Чтобы деструкторы всегда вызывались, каждому *throw* должен соответствовать свой *try/catch*. 

```c++
struct err1 {};
struct err2 {};

struct WithDestructor {
    int key;
    WithDestructor(int key_) : key(key_) {}
    ~WithDestructor() {
        std::cout << "Destructed " << key << "\n";
    }
};

void bar() {
    WithDestructor wd31(31);  // Implementation-defined раскрутка. Может не вызваться деструктор.
    try {
        WithDestructor wd40(40);
        std::cout << "3\n";
        throw err1();
        std::cout << "x3\n";
    } catch (const err1 &e) {
        std::cout << "4\n";
        throw err2();
        std::cout << "x4\n";
    }
    std::cout << "bar end\n";
}

void foo() {
    WithDestructor wd20(20);  // Implementation-defined раскрутка. Может не вызваться деструктор.
    try {
        WithDestructor wd30(30);  // Implementation-defined раскрутка. Может не вызваться деструктор.
        std::cout << "2\n";
        bar();
        std::cout << "x2\n";
    } catch (const err1 &e) {
        std::cout << "z\n";
    }
    std::cout << "foo end\n";
}

int main() {
    std::cout << "1\n";
    {
        WithDestructor wd10(10);
    }
	//foo(); // Деструкторы объектов вырезаются.
	// Т.е. вывод будет какой-то такой: "1\nDestructed 10\n2\n3\nDestructed 40\n4"
    try {
        foo(); 
		// А теперь вывод такой: "1\nDestructed 10\n2\n3\nDestructed 40\n4\nDestructed 31\nDestructed 30\n Destructed 20"
    } catch (...) {
        throw;  // "Перебрось текущее исключение".
    }
    std::cout << "main end\n";
}
```
Также нужно сказать об утечках. Следующий код *Non exception-safe*, переменная *x* не будет удалена.
```c++
int main(){
    try{
	    int *x = new int(10); // Should be unique_ptr.
		foo(); // Код, бросающий исключение
		delete x;
	} catch (const std::exception &err){
	    std::cout << "Exception caught: " << err.what() << "\n";
	}
}
```
Чтобы этого избежать, все выносим в деструкторы.

### Стандартные исключения, наследование, slicing

Существуют стандартные исключения, которые наследуются от *std::exception*. Вот некоторые из них:
```c++
std::exception -> std::logic_error -> std::invalid_argument
               \
			    -> std::runtime_error
```
С помощью них очень удобно ловить исключения и узнавать ошибку через метод *what* у *std::excetions*.
```c++
void bar(int n) {
    if (n % 2 == 0) { throw std::invalid_argument("N is even\n"); }
}

void foo(int n) {
    try { bar(n); } 
	catch (const std::runtime_error &err) {
        std::cout << "Caught runtime_error: " << err.what() << "\n";
    }
}

int main() {
    try {
        foo(2);
    } catch (const std::exception &err) {  // Reference!
        std::cout << "Exception caught: " << err.what() << "\n";
    }
}
```
Если принимать не по ссылке, то будет *slicing* и метод *what* будет уже чем-то непонятным. Более того, *std::exception* \- это полиморфный класс,
его лучше не ловить по значению.

*bad_alloc* - ошибка, которая возникает при выделении памяти, например *vector<int> v(100LL \* 1024 \* 1024 \* 1024)*. Она может вылететь где угодно.

## Exception safety

### noexcept

Если функция не бросает исключений, то нужно пометить её *noexcept*. Также можно создавать собственные исключения:
```c++
struct my_error : std:: exception {
    const char *what() const noexcept override{
        return "my_error";
    }
};
```

### Исключения в деструкторах

Исключения нельзя кидать из деструкторов, потому что непонятно кого обрабатывать, если деструктор вызвался при выбрасывании исключения.
```c++
struct WeirdObject {
    ~WeirdObject() /* noexcept */ {
        throw std::runtime_error("WTF1");
    }
};

int main() {
    try {
        WeirdObject w;
        throw std::runtime_error("WTF2");
    } catch (const std::exception &e) {
        std::cout << "exception: " << e.what() << "\n";
    }
}
```
В таких случаях вся программа завершается, поэтому деструкторы помечаются *noexcept*.

### Безопасность исключений

#### 1. no throw

Код никогда не кидает исключений.
```c++
int a = 10, b = 20;
a = b;
```

#### 2. strong exception safety (сильная гарантия исключений)

Исключение вылетает, но состояние программы от этого не меняется.
```c++
std::vector<int> a(1'000'000);
a.push_back(20);  // Если не получилось, то вектор 'a' не изменился
```

#### 3. basic exception safety

Исключение вылетает, но состояние может быть изменено.
```c++
struct WeirdObject {
    WeirdObject() {}
    WeirdObject(WeirdObject &&) {}
    WeirdObject & operator=(WeirdObject &&) { return *this; }
    WeirdObject(const WeirdObject &) = delete;
    WeirdObject & operator=(const WeirdObject &) = delete;
};
int main(){
    std::vector<WeirdObject> a(1'000'000);
    a.push_back(20); // Если вылетело исключение, то вектор 'a' уже мог измениться
	assert(a.size() == 1'000'000 || a.size() == 1'000'001); // Может быть неверно из-за базовой гарантии.ы
}
```

#### 4. no safety

Очень легко написать код, который не даёт строгой гарантии исключений:
```c++
template<typename T>
struct optional {
    alignas(T) char buf[sizeof(T)];
    bool created = false;
    T& value() { return reinterpret_cast<T&>(buf); }

    ~optional() {
        if (created) {
            value().~T();
            created = false;
        }
    }
	
    optional &operator=(const T &newobj) {
        reset();  // noexcept
        // created = true;  // Здесь располагать неправильно, т.к. следующая строчка может выбросить исключение
		// Тогда вызовется деструктор на несуществующем объекте.
        // Пусть у копирующего конструктора T строгая гарантия исключений.
        new (&value()) T(newobj);
        created = true;  // Здесь располагать правильно.
        return *this;
    }
};
```

### Примеры
#### Нарушение базовой гарантии
```c++
struct VectorInt {
    int *data;
    int length;

    VectorInt(int n = 0)
        : data(n > 0 ? new int[n]() : nullptr)  // Осторожно: n == 0.
        , length(n)
    {
    }

    VectorInt(const VectorInt &other)
        : data(other.length > 0 ? new int[other.length] : nullptr)  // Осторожно: other.length == 0.
        , length(other.length)
    {
        for (int i = 0; i < length; i++)
            data[i] = other.data[i];
    }

    ~VectorInt() {
        delete[] data;  // Осторожно: delete[], а не delete.
    }

    VectorInt &operator=(const VectorInt &other) {
        if (this == &other) {  // Осторожно: иначе удалим данные, а потом новый массив просто скопируем сам в себя.
            return *this;
        }
        length = other.length;  // Здесь вылетит исключение -- получится нарушение инвариантов
        data = new int[other.length]; 
        for (int i = 0; i < length; i++)
            data[i] = other.data[i];
        return *this;
    }
};

int main() {
    VectorInt vec(5);
    vec = vec;  // Злой тест.
}
```

Как просто сделать `strong exception safety`? Сначала сделать копию, сделать с ней все действия, потом поменять инварианты. Так может выйти алгоритмически сложнее, но при этом будет строгая гарантия исключений.

## Неожиданные места, где могут вылетать исключения
### При создании аргументов функции (тогда в `try\catch` надо оборачивать вызов функции, т.к. аргумент создает тот, кто вызывает функцию).
```c++
#include <iostream>

int remaining = 2;

struct Foo {
    Foo() {
        std::cout << "Foo() " << this << "\n";
    }
    Foo(const Foo &other) {
        std::cout << "Foo(const Foo&) " << this << "\n";
        if (!--remaining) {
            throw 0;
        }
    }
    Foo &operator=(const Foo&) = delete;
    ~Foo() {
        std::cout << "~Foo " << this << "\n";
    }
};

void func(Foo a, Foo b, Foo c) {
    std::cout << "func\n";
}

int main() {
    Foo a, b, c;
    try {
        std::cout << "before foo\n";
        func(a, b, c); // при создании второго аргумента (вызовется конструктор копирования) вылетит исключение (внутри конструктора)
        std::cout << "after foo\n";
    } catch (...) {
        std::cout << "exception\n";
    }
}
```

### При возврате значения из функции
```c++
#include <iostream>

int remaining = 1;

struct Foo {
    Foo() {
        std::cout << "Foo() " << this << "\n";
    }
    Foo(const Foo &) {
        std::cout << "Foo(const Foo&) " << this << "\n";
        if (!--remaining) {
            throw 0;
        }
    }
    Foo &operator=(const Foo &) {
        std::cout << "operator=(const Foo&) " << this << "\n";
        if (!--remaining) {
            throw 0;
        }
        return *this;
    }
    ~Foo() {
        std::cout << "~Foo " << this << "\n";
    }
};

Foo func(int x) {
    Foo a, b;
    // Чтобы обхитрить компилятор и нельзя было сделать NRVO так просто.
    if (x == 0) {
        try {
            return a; // вылетает исключение в конструкторе копирования
        } catch (...) {
            std::cout << "exception\n";
            return Foo();
        }
    } else {
        return b;
    }
}

int main() {
    Foo f = func(0);
    std::cout << "&f=" << &f << "\n";
}
```

### В операторе присваивания 
```c++
#include <iostream>

int remaining = 2;

struct Foo {
    Foo() {
        std::cout << "Foo() " << this << "\n";
    }
    Foo(const Foo &) {
        std::cout << "Foo(const Foo&) " << this << "\n";
        if (!--remaining) {
            throw 0;
        }
    }
    Foo &operator=(const Foo &) {
        std::cout << "operator=(const Foo&) " << this << "\n";
        if (!--remaining) {
            throw 0;
        }
        return *this;
    }
    ~Foo() {
        std::cout << "~Foo " << this << "\n";
    }
};

Foo func(int x) {
    Foo a, b;
    // Чтобы обхитрить компилятор и нельзя было сделать NRVO так просто.
    if (x == 0) {
        try {
            return a;
        } catch (...) {
            std::cout << "exception in func\n";
            return Foo();
        }
    } else {
        return b;
    }
}

int main() {
    Foo f;
    try {
        std::cout << "&f=" << &f << "\n";
        f = func(0); // здесь вылетает исключение из оператора присваивания
        std::cout << "after f\n";
    } catch (...) {
        std::cout << "exception in main\n";
    }
}
```
### По этой причине метод pop у `std::stack` не возвращает значение
```c++
#include <vector>

template<typename T>
struct stack {
    std::vector<T> data;

    T pop() {
        T result = std::move(data.back());
        data.pop_back();
        return result;
    }
};

int main() {
    stack<int> s;
    s.data = std::vector{1, 2, 3, 4, 5};

    int x = 10;
    x = s.pop();  // Если вылетело исключение в operator=
                  // то 's' не может восстановить элемент.
                  // Снаружи не выглядит как строгая гарантия.
}
```

Так же тело любой функции можно обернуть в `try\catch` блок. 
### Конструктор (есть тонкости)
Может пригодиться, чтобы обрабатывать исключения в `member init-list`, но обычно так никто не делают (обычно обрабатывают исключения на вызывающей стороне).
```c++
#include <vector>
#include <iostream>

struct Base {
    Base() {
        throw 0;
    }
};

struct Foo : Base {
    std::vector<int> a, b;

    Foo(const std::vector<int> &a_, const std::vector<int> &b_)
    try
        : a(a_)
        , b(b_)
    {
    } catch (const std::bad_alloc &) {
        // Поймали исключение из member initialization list или из тела.
        std::cout << "Allocation failed\n";
        // Поля и базовый класс здесь уже уничтожены.
        // Мы не знаем, откуда вылетело исключение.

        // Объект оживить нельзя. Если не написали throw, 
        // то добавляется автоматически, то есть исключение пробрасывается дальше
    } catch (int x) {
        std::cout << "Oops\n";
        // Базовый класс тоже уже уничтожен.
        throw 10;
    }
};

int main() {
    try {
        Foo f({}, {});
    } catch (int x) {
        std::cout << "exception " << x << "\n";
    }
}
```

### Можем также обернуть деструктор (есть тонкости)
```c++
#include <iostream>

struct Base {
    ~Base() noexcept(false) {
        throw 0;
    }
};

struct Foo : Base {
    ~Foo() try {
    } catch (...) {
        // Ловит исключения не только из тела,
        // но и из деструкторов полей и базовых классов.
        // Они уже уничтожены, обращаться к ним нельзя.

        std::cout << "destruction exception\n";
        // Деструктор не может завершиться корректно, throw и исключение пробрасывается дальше
    }
};

int main() {
    try {
        Foo f;
    } catch (...) {
        std::cout << "exception\n";
    }
}
```

### С обычными функциями 
```c++
#include <iostream>

int remaining = 2;

struct Foo {
    Foo() {
        std::cout << "Foo() " << this << "\n";
    }
    Foo(const Foo &other) {
        std::cout << "Foo(const Foo&) " << this << "\n";
        if (!--remaining) {
            throw 0;
        }
    }
    Foo &operator=(const Foo&) = delete;
    ~Foo() {
        std::cout << "~Foo " << this << "\n";
    }
};

void func(Foo a, Foo b, Foo c) try {
    std::cout << "func\n";
} catch (...) {
    // Всё ещё не ловит исключения при создании параметров.
    // А всё остальное мы можем поймать обычным try-catch.
    std::cout << "exception in func!\n";
}

int main() {
    Foo a, b, c;
    try {
        std::cout << "before foo\n";
        func(a, b, c);
        std::cout << "after foo\n";
    } catch (...) {
        std::cout << "exception\n";
    }
}
```
Последние три способа почти никогда не нужны (информация скорее теоретическая).
# Базовые классы исключений в стандартной библиотеке.
* __logic_error__ и __runtime_error__ - стандартные классы, от которых можно наследовать свои собственные исключения. Они расположены в хедере `<stdexcept>` и имеют следующее "логическое" разделение (которое впрочем не является строгим):
  
    * __logic_error__ - нарушение инвариантов, которые можно найти до запуска программы в процессе чтения кода. 
    * __runtime_error__ - ошибки, соответствующие, к примеру, некорректному вводу пользователя.
    
* Далее продемонстрированы способы написать своё исключение, наследуясь от `std::exception`:
    * Обычный `std::excpetion` обязан предоставлять виртуальный метод `what()`, который по умолчанию возвращает сишную строку _"std::exception"_. Если же 
    мы хотим сделать своё сообщение об ошибке, то мы можем это сделать, перезаписав метод `what()`. Но сложность в данном случае заключается в том, что `what()` возвращает `const char *`, поэтому мы можем вернуть только какую-нибудь строковую константу и только её:
      
        ```C++
      struct my_error : std::exception {
            const char *what() const noexcept override {
                return "hello";  // ok
                //        return std::to_string(10).c_str();  // UB, но прячется (из-за Small String Optimization?)
                //        return std::string(100, 'a').c_str();  // UB, меньше прячется.
            }
        };  
      int main() {
          try {
                throw my_error();
              } catch (const std::exception &e) {
                std::cout << e.what() << "\n";
          }
      }
        ```
    * `std::string` вернуть не получится, поскольку для него не существует преобразования в `const char *`
    * У `std::string` есть специальный метод `c_str()`, который применяется в похожих случаях. Такой код скомпилируется и запустится и выведет на экран _"10"_, но на самом деле это UB. Дело в том, что 
    метод `c_str` возвращает указатель на первый элемент строки и убеждается, что в конце есть ноль. Тогда получается, что мы создали `to_string(10)`, взяли указатель на нулевой элемент и вернули его из 
      функции `what()`, но `string` здесь является временным объектом, при этом мы вернули не сам временный объект, а указатель, поэтому временный объект умирает при ближайшей `;` и освобождает память, которую занимал.
      Получается, что мы вернули указатель на уже умерший объект, следовательно, это UB. Если же взять строку по-больше, то она уже выделится в динамической памяти и UB проявится во всей красе.
      
    * Мы можем добавить в `my_error` строковое поле:
        ```C++
        struct my_error_2 : std::exception {
            char str[10] = "msg?";
        
            my_error_2(int x) {
                assert(0 <= x && x <= 9);
                str[3] = '0' + x;
            }
        
            const char *what() const noexcept override {
                return str;  // ok
            }
        };
        ```
    * А можем хранить `std::string` в структуре (теперь мы можем удобно пользоваться строкой и при этом указатель, который мы будем возвращать из `what()`, будет жить столько же, сколько живёт `my_error_3`). Однако оператор равенства должен быть `noexcept`, поскольку компилятор внутри
      себя может копировать исключения несколько раз:
        ```C++
        struct my_error_3 : std::exception {
            std::string str;  // Oops: operator=() должен быть noexcept, но не может.
        
            my_error_3(int x) : str("hello " + std::to_string(x)) {
            }
        
            const char *what() const noexcept override {
                return str.c_str();  // ok
            }
        };
        ```
      
* Но легче всего наследоваться от `runtime_error`, у него уже есть конструктор от `std::string` и корректно переопределённый метод `what()`:
    ```C++
    struct my_error_4 : std::runtime_error {
        // Ok.
        my_error_4(int x) : std::runtime_error("hello " + std::to_string(x)) {
        }
    };
    
    int main() {
        try {
            throw my_error_4(123456);
        } catch (const std::exception &e) {
            std::cout << e.what() << "\n";
        }
    }
    ```
  Если углубиться в технические детали, то внутри себя `runtime_error` обернёт строку во что-то вроде `shared_ptr`, но чуть более эффективное.

# "Скорость работы" исключений
* Рассмотрим пример такого кода:
    ```C++
    // Разницы от наличия noexcept на этих примерах не получилось, но она иногда бывает:
    // 1. Привет от не-noexcept-move и строгой гарантии.
    // 2. Можно оптимизировать код вроде `i++; my_noexcept(); i++;`
    // 3. Не надо готовиться к раскрутке стэка.
    //
    // Стоит помечать, аналогично const.
    //
    // Разницы от отключения исключений тоже локально не нашлось.
    
    bool foobar(int a, int b, bool please_throw) noexcept {
    /*    if (please_throw) {
        if (a % b) {
            throw 0;
        }
            return true;
        }*/
        return a % b == 0;
    }
    
    const int STEPS = 100'000'000;
    
    int main() {
    auto start = std::chrono::steady_clock::now();
    
        for (int i = 0; i < STEPS; i++) {
    //        try {
                foobar(10, 3, false);
    //        } catch (...) {
    //        }
    }
    
        auto duration = std::chrono::steady_clock::now() - start;
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << "ms\n";
    }
    ```
  * Замерим количество времени, которое будет портачено на выполнение 100'000'000 операций. Такой код выполнится очень быстро - за 0ms (компилятор соптимизировал бесполезный цикл и ничего не выполнял). Если отключить оптимизатор, то окажется, 
  что на самом деле это работает за 491ms. Теперь изменим код так, чтобы функция кидала исключение, поймаем его и ничего не будем с ним делать, уменьшим количество вызовов функции до 1'000'000. Итого получим время работы 1700ms (замедление в 350 раз).
  Замедление возникает только когда мы бросаем исключение и ловим его (то есть если даже есть команды `throw`, но они не выполняются или есть блок try-catch, но исключения не кидаются, то замедления не возникает).
  
  * Замедление возникает из-за того, что компилятор начинает раскручивать стэк и анализировать, какие деструкторы надо вызвать и т.д.
  * Если можно добваить к функции `noexcept`, то лучше его добавить, это может улучшить время работы программы (к примеру, вектор может вести себя по-разному в зависимости от того, является ли move оператор `noexcept` или нет, компилятору не нужно готовиться к раскрутке стэка).
  * Необходимо использовать исключения только для обработки "исключительных ситуаций".
  * Чтобы отрубить исключения полностью во всей программе можно добавить ключ компиляции `-fno-exceptions`

### Иногда исключения в проектах запрещают полностью.
Тогда сложно взаимодействовать со стандартной библиотекой.

#### Причины:

* Так сложилось исторически, код не был готов к исключениям.
* Исключения могут быть не полностью zero-overhead.
* Несовместимость ABI: если библиотека не поддерживает, может быть упс.

# Выбрасывание исключений из делегирующего конструктора.

```C++
struct Foo {
    int x = 10;
    Foo(int) {
        std::cout << "Foo(int) a\n";
//        throw 0; 
        std::cout << "Foo(int) b\n";
    }

    Foo(int, int) : Foo(10) {
        std::cout << "Foo(int, int) a\n";
//        throw 0;
        std::cout << "Foo(int, int) b\n";
    }

    Foo(int, int, int) : Foo(10, 20) {
        std::cout << "Foo(int, int, int) a\n";
        throw 0;
        std::cout << "Foo(int, int, int) b\n";
    }

    ~Foo() {
        std::cout << "~Foo()\n";
    }
};

int main() {
    try {
        Foo f(10, 20, 30);
    } catch (...) {
        std::cout << "exception\n";
    }
}
```

* Если мы бросаем исключение в обычном конструкторе, то деструктор объекта не вызывается, поскольку в таком случае объект считается не созданным. Конструктор отвечает за то,
чтобы либо полностью создать объект, либо полностью не создать.
  
* При вызове делегирующего конструктора от двух интов (см. код) аналогично будет выброшено исключение и объект всё так же не создастся, поскольку первый конструктор не завершился.

* __Внимание!__ если в делегирующем конструкторе завершился "вложенный конструктор" (в данном случае конструктор от одного инта), то считается, что объект уже создан. Следовательно, выведется деструктор, и только потом выведется строчка _"exception"_

  
# Исключения (лекции `27-210513` и `29-210527`)

## std::exception_ptr

`exception_ptr` это что-то типа `shared_ptr<exception>`
Создание объекта --- `make_exception_ptr`
Еще `exception_ptr` можно получить вызвав `std::current_exception`. Если мы находимся внутри блока `catch`, то получим `exception_ptr` на текущее исключение (при этом `catch` может даже ничего не знать про то что он поймал), если же мы находимся вне какого-то блока `catch`, то сохранится пустой `exception_ptr{}`. (Видимо, он же `nullptr`).
Для чего можно использовать:
- например в одном месте программы сохранить в `std::exception_ptr err` какое исключение выкидывается, что-нибудь сделать в программе и потом снова выкинуть это же исключение используя `std::rethrow_exception(err)`

```c++
std::exception_ptr err;  // ~ shared_ptr<exception>
// std::make_exception_ptr
auto save_exception = [&]() {
    // В любой момент программы, необязательно непосредственно внутри catch.
    // Если нет текущего исключения - возвращает exception_ptr{}.
    err = std::current_exception();
};

try {
    foo(1);
} catch (...) {
    save_exception();
    // Out-of-scope: можно сделать nested exceptions: https://en.cppreference.com/w/cpp/error/nested_exception
}

try {
    if (err) {
        std::rethrow_exception(err);  // Требует непустой err.
    } else {
        std::cout << "no exception\n";
    }
} catch (my_exception &e) {
    std::cout << "e=" << e.value << "\n";
}
```

- Механизм с перебрасыванием `exception_ptr` (вроде бы) используется внутри `std::future` и `std::assync` (вполне себе задачка на экзамен). 

```c++
auto f = std::async([]() {
    foo(0);  // выбрасывает внутри себя исключение
});

try {
    f.get();  // вызывает foo(0)
} catch (my_exception &e) {
    std::cout << "e=" << e.value << "\n";
}
```

- [[Extra]] вне билета: Еще можно с помощью `exception_ptr` можно делать nested exceptions --- вложенные исключения. (Кинули исключение, поймали его и завернули в еще одно и т.д.). Активно используется в Java, в C++ Егор Суворов особо не видел использований.

## Свойства выражений: `noexcept`
Про типы всё можем узнавать, теперь давайте что-нибудь узнаем про выражения и функции.

* Оператор `noexcept` позволяет узнать, может ли из выражения теоретически
  вылететь исключение (по спецификаторам `noexcept`).
* Возвращает `true`/`false` _на этапе компиляции_, не вычисляя выражение.


```c++
int foo() noexcept { return 1; }
int bar()          { return 2; }
// ....
int a = 10;
vector<int> b;
static_assert(noexcept(a == 10));
static_assert(!noexcept(new int{}));   // Утечки не будет: не вычисляется.
static_assert(noexcept(a == foo()));
static_assert(!noexcept(a == bar()));  // bar() не noexcept
static_assert(!noexcept(b == b));      // vector::operator== не noexcept
bool x = noexcept(a = 10);
assert(x);
```

---
## Условный `noexcept`
У _спецификатора_ (не оператора) `noexcept` есть две формы:

```c++
template<typename T> struct optional {
    optional() noexcept;  // Не кидает никогда
    optional(optional &&other) noexcept(  // noexcept только при условии...
        std::is_nothrow_move_constructible_v<T>
    );
};
```

Можно вызвать оператор внутри спецификатора:
```c++
template<typename T> struct optional {
    optional() noexcept;
    optional(optional &&other) noexcept(    // noexcept только при условии...
       noexcept(  // ...что следующее выражение noexcept...
           T(std::move(other))  // вызов move-конструктора T
       )
   );
}
```

Получаем:
```c++
template<typename T> struct optional {
    optional() noexcept;
    optional(optional &&other) noexcept(noexcept(T(std::move(other))));
}
```

---
## Применение `noexcept`
Полезно для `vector` со строгой гарантией исключений.

Если у элементов `is_nothrow_move_constructible`, то можно перевыделять буфер
без копирований:

```c++
void increase_buffer() {
    vector_holder new_data = allocate(2 * capacity);  // Может быть исключение.
    for (size_t i = 0; i < len; i++)
        new (new_data + i) T(std::move(data[i]));     // Портим data, боимся исключений.
    data.swap(new_data);                              // Исключений точно нет.
}
```

Иначе мы обязаны копировать.
Уже есть готовая функция:

```c++
void increase_buffer() {
    vector_holder new_data = allocate(2 * capacity);
    for (size_t i = 0; i < len; i++)
        new (new_data + i) T(std::move_if_noexcept(data[i]));
    data.swap(new_data);
}
template<typename T> auto move_if_noexcept(T &arg) {  // Упрощённо
    if constexpr (std::is_nothrow_move_constructible_v<T>)
        return std::move(arg);  // Перемещаем аргумент в возвращаемое значение
    else
        return arg;             // Копируем аргумент в возвращаемое значение
}
```
