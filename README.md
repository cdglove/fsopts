# fsopts

fsopts is a small helper library to facilitate live tunables in code. It's primary use is for situations where tweaking a tuneable using a debugger is difficult, causes timing issues, or is simply unavailable.

Example:

```cpp
if(g_dump_all_things) {
  for(auto&& thing : object.get_all_the_things()) {
    std::cout << thing;
  }
}
```

In this very simple example, one might be manually running a test and need to debug whats in the thing collection. Toggling `g_dump_all_things` in the debugger is one way to engage this code path, but in scenarios where that's difficult, and the system doesn't already have a machanism for tweaking variables, we can use this library.

```cpp
static fsopts::Description opts("/tmp");
static fsopts::Option<bool> dump_all_things = opts.add("dump-all-things", fsopts::Trigger());
opts.update();

if(*dump_all_things) {
  for(auto&& thing : object.get_all_the_things()) {
    std::cout << thing;
  }
}
```

We can then set the `Option<bool>` by touching the correct file:

```sh
$ touch /tmp/dump-all-things
```

When `Description::update` is called, it will detect the presense of the file, set the bool to true, and delete the file. The next call to update will set the bool back to false.

In scenarios where we want to use the existing variable, we can modify the code as follows:

```cpp
static fsopts::Description opts("/tmp");
static fsopts::Option<bool> dump_all_things = opts.add("dump-all-things", fsopts::Trigger(&g_dump_all_things));
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
static fsopts::Option<int> count = opts.add("dump-some-things", fsopts::Value<int>().default_value(0));
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

# Design rationale

The design relies on the user creating static variables to track the Description and Option instances. This was done intentionally do satisy the following requirements:
    
* **Code should integrate easily into any piece of existing code.** The idea is that this is a debugging & testing tool, and the use might not have access to other code to do a full integration. Assigning to statics makes this easy, and made the utilisation of a Option class obvious.
    
* **Reading variables should be fast.** We don't want to affect the speed of the surrounding code too much in case the user is toggling featuers for performance testing, hech we don't want to to be doing things like hash table lookups in the hot path.
