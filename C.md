# С

> Автор: Шеремеев Андрей

## 17-210209

### Арифметика указателей 

```C++
struct Base {
    int x, y;
};

int main() {
    Base b[4]{
       {100, 200},
       {300, 400},
       {500, 600},
       {700, 800}
    };
    std::cout << &b << "\n"; //адресс массива и [0] элемента - совпадают
    std::cout << &b[0] << "\n";
    std::cout << &b[1] << "\n";
    std::cout << &b[2] << "\n";

    Base *bptr = &b[0]; //заводим указатель и вручкую сдвигаем его 
    					//не обязательно его заводить на первый элемент
    std::cout << bptr << "\n";
    std::cout << bptr + 1 << "\n";
    std::cout << bptr + 2 << "\n";
}
```

**[]** - синтаксический сахар над арифметикой указателей `a[b] ~~ *(a+b)`
```C++
	Base *bptr2 = &b[1];
    std::cout << bptr2 - 1 << "\n";
    std::cout << bptr2 << "\n";
    std::cout << bptr2 + 1 << "\n";

    std::cout << bptr2[0].x << " " << bptr2[0].y << "\n";  // (*(bptr2 + 0)).x ~~ (*&b[1]).x --- 300 400
    std::cout << bptr2[-1].x << " " << bptr2[-1].y << "\n";  // (*(bptr2 - 1)).x ~~ (*(&b[1] - 1)).x ~~ (*&b[0]).x --- 100 200

```

страшилки (не нужно так писать)
```C++
 	std::cout << (-1)[bptr2].x << "\n";  // (*(-1 + bptr2)).x ---1000
    std::cout << 1[bptr2].x << "\n";  // (*(1 + bptr2)).x --- 500
```

Указатель массива совпадает с указателем на первый элемент, но типы это разные (**нельзя**, например, присвоить)
```C++
    Base *bp1 = &b;  // не компилируется: &b - это "указатель на массив", а не "указатель на элемент".

```

##### Array-to-pointer decay
можно считать, что это неявное преобразование
```C++
 	Base *bp2 = b; //но вот так уже делать можно, wtf

	/*
	пример:
	 int arr[]{1, 2, 3, 4};
     std::sort(arr, arr + 4); 
неявная конвертация arr в указатель на 1 элемент, и указатель на после последнего, потом сортилось 
*/

 	auto *bp3 = b;
    auto bp4 = b;  // auto тоже делает array-to-pointer decay; массивы-то копировать нельзя.

    std::cout << boost::core::demangle(typeid(bp2).name()) << "\n"; // Base*
    std::cout << boost::core::demangle(typeid(bp3).name()) << "\n"; // Base*
    std::cout << boost::core::demangle(typeid(bp4).name()) << "\n"; // Base*

```

##### One-past-last

```C

int main() {
    int arr[4];

    int *x0 = &arr[0];	// OK
    int *x3 = &arr[3]; 	// OK
    int *x4 = &arr[4];  // UB: не существует arr[4].

    int *p = arr; //уже знаем, что это неявное преобразование (array-to-pointer decay)
    int *p0 = p + 0;
    int *p3 = p + 3;
    int *p4 = p + 4;  // OK: указатель на one-past-last брать можно. Но разыменовывать нельзя.
    *p4;  // UB
    int *p5 = p + 5;  // UB (как только получили выход за пределы, с этого момента началось UB)
    int *p5b = p + 5 - 1;  // UB: (p + 5) - 1
    int *p5c = p + (5 - 1);  // OK: p + 4
    int *p0b = p - 1 + 1;  // UB: (p - 1) + 1
}
```
##### Преколы с Void 

`void *` - указатель куда-то, разыменовывать нельзя - CE (compile error) 

любой указатель можно -> void \*, но чтобы сделать обратный переход, нужен `static_cast`

Зачем - для полиморфизма 

Применение в C++ - [аллокатор памяти] нужно выделить кусок памяти, но не создавать объекты на нем (использовалось, например, в `lab07-vector`, когда мы хотели создать объект, а выделять память по мере необходимости - для этого нужно было разделить 2 действия - выделения памяти и создание объекта (то что совмещает в себе new/delete))

```C++
 // В C++ почти не используется, используется в C.
    void *x = arr;
    // *x;
//    static_cast<int*>(x)
    x++;  // O_O: расширение GCC: работает, как с char* (есть арифметика указателей, но по стандарту так делать нельзя!)

```

## 19-210302

### Aliasing 

**Aliasing** (псевдонимы/наложение/алиасинг) - это ситуация, когда два разных имени (например указателя) обозначают один и тот же объект.

#### Reinterpret-cast 

```C
	int x = 123456;
    char *xc = reinterpret_cast<char*>(&x);
    // Разворачивается в такую штуку:
    // char *xc = static_cast<char*>(static_cast<void*>(&x));
    // один 'static_cast' тут бы не помог 
    for (std::size_t i = 0; i < sizeof(x); i++) {
    	//выводим байты
        std::cout << static_cast<int>(xc[i]) << "\n";
    }
```

Пример того, как NOT OK - нельзя смотреть на объекты через указатель другого типа (исключение -char)
```C
	float f = 1.0;
	static_assert(sizeof(float) == sizeof(int));

    int *x = reinterpret_cast<int*>(&f);
    std::cout << std::hex << *x /* UB */ << "\n";

    // Strict aliasing rule:
    // Можно через указатель p типа T1 обращаться к объекту типа T2 только если:
    // 1. T1 == T2 (но T1 может быть более const)
    // 2. T1 --- базовый класс T2
    // .....
    // 10. T1 == char

    // Выше нарушаем: T1 == int, T2 == float.

    // Тут тоже нарушение с точки зрения C++: https://en.wikipedia.org/wiki/Fast_inverse_square_root
    // Но, возможно, не с точки зрения C.

```

Зачем такой UB  нужно - очев для оптимизация (wtf 2.0)

Как тут работает strict aliasing rule - он оптимизирует вывод в `return 10`, при этом гарантирует, что `*b`  никак не поменяет a, то есть нельзя к нему обратиться через `b`. 
```C
int func(int *a, float *b) {
   *a = 10;
   *b = 123.45;
   return *a;  // --> return 10;
}
```

Пример нарушения:
Тут отработает корректно и вывод будет `10, 10, 123.45` 
```C
 	{
        int a = 15;
        float b = 456.78;
        int res = func(&a, &b);
        std::cout << "res=" << res << "\n";
        std::cout << "a=" << a << "\n";
        std::cout << "b=" << b << "\n";
	}

```
А вот тут в `a` запишется полная дичь, потому что `reinterpret_cast` как раз нарушает aliasing
```C
	{
        int a = 15;
        int res = func(&a, reinterpret_cast<float*>(&a));
        std::cout << "res=" << res << "\n";
        std::cout << "a=" << a << "\n";
    }

```

Но если мы заменим `float` -> `char`, то работать будет как в примере с выводом байт, тк через указатель на char можно обращаться к чему угодно 

Можно еще менять байтики 
```C
 	int x = 123456;
    char *xc = reinterpret_cast<char*>(&x);
    static_assert(sizeof(int) == 4);

    xc[0] = 10;
    xc[1] = 11;
    xc[2] = 12;
    xc[3] = 13;
    std::cout << std::hex << x << "\n";
```

А так делать плохо, потому что конвертируем в `int`, а на самом деле это массив `char`
```C
	char xc[] = {10, 11, 12, 13};
    static_assert(sizeof(int) == 4);

    int *xptr = reinterpret_cast<int*>(xc);
    std::cout << std::hex << *xptr /* UB */ << "\n";
    // T1 == int, T2 == char. Нельзя.

```

**Вывод** - через `char` можно объекты менять и читать, но читать `char` как объекты - нельзя 

Пример правильно конвертации `float` -> `int` (важно, что это `char*`) (аналог `std::memcpy`)
```C
	float x = 1.0;
    int y;

    static_assert(sizeof(x) == sizeof(y));
    // Аналог std::memcpy. Не UB.
    // Начиная с C++20 есть bit_cast<>.
    for (int i = 0; i < 4; i++) {
        reinterpret_cast<char*>(&y)[i] = reinterpret_cast<char*>(&x)[i];
    }

    std::cout << std::hex << y << "\n";
```

### Plain-old-data 
Нет виртуальности, нет нетривиальных конструкторов/деструкторов/полей/базовых классов.
Простые базовые классы могут быть.

Применение - в `C` сохраняют данные на диск 

Пример такой структурки
```C
struct MyPod {
    int x = 10;
    char y = 20;
    // Компилятор добавил padding: 3 левых байта.
    // А мог и не добавлять (зависит от компилятора)
    float z = 30;
};

int main() {
    MyPod p;
    //создаем бинарный файл
    std::ofstream f("01.bin", std::ios_base::out | std::ios_base::binary);
    // Не UB.
    //передаю указатель "что записывать" и "сколько"
    f.write(reinterpret_cast<const char*>(&p), sizeof(p));
    //аналогично можно считать данные из файла в стурктурку
}
```

В файл запишется `0A 00 00 00 14 10 28 FF | 00 00 F0 41`
Непонятно откуда взялись `10 28 FF`, тк в 14 - это 20, а в конце float -> добавилось выравнивание до 4 байт (можно установить свое, написав перед переменными `alignas(8)`)


У компиляторов есть стек параметров выравнивания, в который можем сами сказать, сколько и чего с помощью `#pragma pack(push, 1)` - выравнивай в этой блоке все по 1-ому байту `#pragma pop()`

```C
#pragma pack(push, 1)
struct MyPod {
    int x = 10;
    char y = 20;
    float z = 30;
};
#pragma pack(pop)

```
Теперь нет padding, меньше зависим от ключей компиляции

Но всё ещё проблемы с: 
1. Порядок байт (x86 little-endian; старые Mac, некоторые роутеры на 'mips', но не 'mipsel').
2. Не все процессоры умеют читать невыровненную память (ARM, но я не специалист).
3. Точные размеры int/float - могут быть разными 

Про `std::string` - работать не будет в такой структуре, это не тривиальный тип и непонятно куда указатели у него 

## 20-210309

### c-str

Конвенция для хранения строк - массив байт `char arr[]`
Массив байт всегда(!) заканчивается нулевым символом, поэтому размер такого массива всегда (+1), тк мы можем завести указатель на строчку и вывести нашу строку (а ему нужно знать, куда ссылаться по памяти)

```C
	char arr[] = "Hello";
	//   char arr[] = {'H', 'e', 'l', 'l', 'o', 0 /*'\0'*/};
    std::cout << sizeof(arr) << "\n";

    char *str = arr;
    std::cout << str << "\n";

```

Если не указывать `0` в конце - UB. 

Тут работает арифметика указателей - тогда выведется суффикс строки 

```C
	char arr[] = "Hello";
	char *str = arr + 2;
	std::cout << str << "\n"; // llo
```

На самом деле тип строкового литерала (это не `char*` / `const char*`) - это `char [6]` массивы `char` размера - длина строки + 1
```C
    std::cout << boost::core::demangle(typeid("Hello").name()) << "\n";

```

Но могут быть проблемы, если вставить символ `\0` в середину строки.
В случае, когда мы выводим всю строку - нулевой символ считается пробелом 
```C
 	char arr[] = "Wow\0Foo";
	std::cout << sizeof(arr) << "\n"; 									// 8
	std::cout << arr << "\n";											// Wow
	std::cout << std::string(arr) << "\n";								// Wow
	std::cout << std::string(arr, arr + sizeof(arr)) << "\n"; 			// Wow Foo
	std::cout << std::string(std::begin(arr), std::end(arr)) << "\n";   // Wow Foo
	std::cout << arr + 4 << "\n";										// Foo 
```

##### std::strlen
```C
	char arr[] = "Hello World";
	std::cout << std::strlen(arr) << "\n"; 		// 11
	std::cout << std::strlen(arr + 3) << "\n"; 	// 8

	// реализация 
	auto my_strlen = [&](const char *s){
    	int len = 0;
    	while (s[len] != 0) {
    	  len++;
   		}
    	return len;
  	};

	std::cout << my_strlen(arr) << "\n"; 		// 11
	std::cout << my_strlen(arr + 3) << "\n";	// 8
```


##### std::strcpy
иногда хочется суеты и написать свой `strcpy`, на заметку, он пишется вот так - 

```C
void my_strcpy(char *dest, const char *src) {
    for (int i = 0;; i++) {
        dest[i] = src[i];
        if (!src[i]) { 		//скопировали нулевой байт
            return;
        }
    }
}

void run(char *foo, char *bar, char *baz) {
    my_strcpy(foo, bar);  // foo = bar
    std::cout << "|" << foo << "|" << bar << "|\n"; // World World

    my_strcpy(foo, baz);
    std::cout << "|" << foo << "|\n"; 				// Hi 
    												// если не было if(!src[i]), было бы 'Hirld'

    my_strcpy(foo, "12345");  // OK.				// 12345
    std::cout << "|" << foo << "|\n";

    my_strcpy(foo, "123456");  // UB! (выход за границы массива) Но мы про это не знаем.
    std::cout << "|" << foo << "|\n";

    std::strcpy(foo, "123456");  // UB! Но мы про это не знаем.
    std::cout << "|" << foo << "|\n";
}

int main() {
    char foo[] = "Hello";
    char bar[] = "World";
    char baz[] = "Hi";
    run(foo, bar, baz);
}

```

Как обычно лечат это UB? - с помощью `strncpy` (но все равно не безопасно, тк могут размеры поехать)

```C
void my_strncpy(char *dest, const char *src, int n) {
    for (int i = 0; i < n; i++) {  	// <n 
        dest[i] = src[i];
        if (!src[i]) {
            return;
        }
    }
}

int main() {
    char foo[] = "Hello";
    my_strncpy(foo, "123456", sizeof foo);  // UB отсутствует.
    std::cout << foo << "\n";  // UB! Строчка не null-terminated.

    strncpy(foo, "123456", sizeof foo);  // UB отсутствует.
    std::cout << foo << "\n";  // UB! Строчка не null-terminated.
}
```

Прелесть в том, что можно поделить строку, вставив на проебельные символу 0, без перевыделения памяти (с `std::vector` такое не прокатит)
```C
   char command_line[] = "arg1 arg2 arg3";
    // std::strtok занимается примерно этим.
    for (int i = 0; command_line[i]; i++) {
        if (command_line[i] == ' ') {
            command_line[i] = 0;
        }
    }

    std::vector<char*> args = {
        command_line + 0,
        command_line + 5,
        command_line + 10,
    };
    std::cout << args[0] << "\n"; // arg1
    std::cout << args[1] << "\n"; // arg2
    std::cout << args[2] << "\n"; // arg3
```

`sizeof(argv[i])` - берет размер типа, то есть это равно `sizeof(char*`, так вообще делать нельзя, если строчка не `const`!, если нужен размер - `strlen`
```C
	for (int i = 0; i < argc; i++) {
        std::cout << "argv[]="
                  << argv[i]
                  << "; strlen="
//                  << sizeof(argv[i])  // Так нельзя делать никогда! Это sizeof(char*)
                  << std::strlen(argv[i])
                  << "; p="
                  << static_cast<void*>(argv[i]) << "\n";
    }
```


###### Конкатенация

 так делать нельзя - складывать 2 указателя - бред
 ```C
 /*const*/ char* a = "hello ";  // Время жизни строкового литерала - static, управляет компилятор.
    // a[0] = 'x';  // UB.

    const char* b = "world";
    // std::cout << (a + b) << "\n";
 ```










