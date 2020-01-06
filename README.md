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

In this very simple example, One might be manually running a test, and need to debug what's in the thing collection. Toggling `g_dump_all_things` in the debugger is one way to engage this code path, but in scenarios where that's difficult, and the system doesn't already have a machanism for tweaking variables, we can use this library.

```cpp
static fsopts::Description opts("/tmp");
static fsopts::Handle<bool> dump_all_things = opts.add("dump-all-things", fsopts::Trigger());
opts.update();

if(*dump_all_things) {
  for(auto&& thing : object.get_all_the_things()) {
    std::cout << thing;
  }
}
```

We can then set the `Handle<bool>` by touching the correct file:

```sh
$ touch /tmp/dump-all-things
```

When `Description::update` is called, it will detect the presense of the file, set the bool to true, and delete the file.

In scenarios where we want to use the existing variable, we can modify the code as follows:

```cpp
static fsopts::Description opts("/tmp");
static fsopts::Handle<bool> dump_all_things = opts.add("dump-all-things", fsopts::Trigger(&g_dump_all_things));
opts.update();

if(g_dump_all_things) {
  for(auto&& thing : object.get_all_the_things()) {
    std::cout << thing;
  }
}
```

This is the behaviour of the Trigger options, but we can also read values from files to pass tweakable information in. Expanding on the previous example, maybe we want to limit the output to the first N values:

```cpp
static fsopts::Description opts("/tmp");
static fsopts::Handle<int> count = opts.add("dump-some-things", fsopts::Value<int>());
opts.update();

int dump_count = *count;
for(auto&& thing : object.get_all_the_things()) {
  if(dump_count-- == 0) {
    break;
  }
  std::cout << thing;
}
```

In this case, we can set the value by writing to the correct file.

```sh
$ echo 3 > /tmp/dump-some-things
```
