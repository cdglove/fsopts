# fsopts

fsopts is a small helper library to faciliate live tunablaes in code. It's primary use is for situations where tweaking a tunable using a debugger is difficult, causes timing issues, or is simply unavailable.

Example:

```cpp
if(g_dump_all_things) {
  for(auto&& thing : object.get_all_the_things()) {
    std::cout << thing;
  }
}
```

In this very simple example, I might be manually running a test, and need to debug what's in the thing collection. Normally, I might toggle `g_dump_all_things` in the debugger to engage this code path. In scenarios where that's difficult, and the system doesn't already have a machanism for tweaking variables, we can use this library.

```cpp
static fsopts::Description opts("/tmp");
static dump_all_things = opts.add("dump-all-things", fsopts::Value<bool>());
opts.update();

if(*dump_all_things) {
  for(auto&& thing : object.get_all_the_things()) {
    std::cout << thing;
  }
}
```

In scenarios where we want to use the existing variable, we can modify the code as follows.


```cpp
static fsopts::Description opts("/tmp");
static dump_all_things = opts.add("dump-all-things", fsopts::Value<bool>(&g_dump_all_things));
opts.update();

if(g_dump_all_things) {
  for(auto&& thing : object.get_all_the_things()) {
    std::cout << thing;
  }
}
```
