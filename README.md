# Compiler for the void language

## Preamble
void is a high-level, statically typed, compiled language that compiles to LLVM IR. It is focused on developer ergonomics and speed, while maintaining low cpu overhead and a simple mental model.

"void" means absence - the absence of barriers and prescription.

Currently this repository is something like a prototype - it's a working example to get a feel for the language and explore ideas around it - feel free to play around with it but note that it is very rough and ready.

## Current features!

### Functions!
```void
import fmt

// "const" in void means "compile time"
const add = fn(x: i32, y: i32) -> i32 {
  return x + y
}

// you can also omit curly braces with the "do" keyword followed by an expression
const multiply = fn(x: i32, y: i32) -> i32 do return x * y

const main = fn() {
  fmt.println("1 + 2 == {:d}", add(1, 2))
  fmt.println("10 * 20 == {:d}", multiply(10, 20))
}
```

output: 
```
1 + 2 == 3
10 * 20 == 200
```

### Control flow!
```void
const pow = fn(base: i32, exponent: i32) -> i32 {
  if exponent == 0 do return 1

  result: i32 = 1
  loop i in 0..exponent do result = result * base
  return result
}

const main = fn() {
  fmt.println("2 ^ 0 == {:d}", pow(2, 0))
  fmt.println("2 ^ 1 == {:d}", pow(2, 1))
  fmt.println("2 ^ 2 == {:d}", pow(2, 2))
  fmt.println("2 ^ 3 == {:d}", pow(2, 3))
  fmt.println("2 ^ 4 == {:d}", pow(2, 4))
  fmt.println("2 ^ 5 == {:d}", pow(2, 5))
  fmt.println("2 ^ 6 == {:d}", pow(2, 6))
  fmt.println("2 ^ 7 == {:d}", pow(2, 7))
  fmt.println("2 ^ 8 == {:d}", pow(2, 8))
  fmt.println("2 ^ 9 == {:d}", pow(2, 9))
  fmt.println("2 ^ 10 == {:d}", pow(2, 10))
  fmt.println("2 ^ 11 == {:d}", pow(2, 11))
  fmt.println("2 ^ 12 == {:d}", pow(2, 12))
}
```

output: 
```
2 ^ 0 == 1
2 ^ 1 == 2
2 ^ 2 == 4
2 ^ 3 == 8
2 ^ 4 == 16
2 ^ 5 == 32
2 ^ 6 == 64
2 ^ 7 == 128
2 ^ 8 == 256
2 ^ 9 == 512
2 ^ 10 == 1024
2 ^ 11 == 2048
2 ^ 12 == 4096
```

## Getting Started
Since this project uses LLVM you'll have to install LLVM version 20, and I'd recommend using the clang compiler

To build and run:
```sh
cmake -Bbuild -DCMAKE_CXX_COMPILER=clang++
cmake --build build
./build/void_compiler build void.main
./a.out
```

The following sections outline upcoming features.

## Planned Features
Currently void only supports `i32`'s. This is ok because `3 + 2 == 5` and 5 represents Geburah in the Kabbalistic tree of life, meaning strength and severity!

NOTE: All the below code examples don't compile, they're just theoretical for now

In the near future void will support the following types:

### Primitive types
* Slices: `[]T` 
* All sized ints: `i8, i16, i32, i64, u8, u16, u32, u64` 
* Strings (slices of `u8`'s): `string` 
* Booleans: `bool`

### User defined types
#### Structs 

```void
const User = struct {
  id: i64,
  name: string,
  hashed_password: string,
}

// Methods on types:
const User.make = fn() -> User {
  return User {
    .id = generate_user_id()
    .name = 
  }
}

const main = fn() {
  user := User.make()

  // runtime & compile time reflection is also planned
  fmt.println("user: {}", user)
}
```

#### Unions
Tagged unions

```void
const Response = union {
  user: User,
  error: string,
}

const main = fn() {
  response: Response = "No user found!"

  // this match syntax is unconfirmed, I may find a better way
  match response {
    case Response.user do fmt.println("User found: {}", response.user)
    case Response.error do fmt.println("Error found: {}", response.error)
  }
}
```

#### Memory Model
void will not have a Garbage Collector or a Borrow Checker - GC adds too much runtime overhead and BC adds mental overhead. The idea of void is to be a fast, simple, productive language.

To achieve this, memory will be managed by a combination of move-only "owned" pointer types which deallocate at the close of owning scope, and "tagged" pointers, which panic at runtime if temporal memory errors like Use After Free are detected.

Tagged pointers contain a pointer to the data and a "tag" which acts as a key to the memory. When you attempt to dereference memory, it will only succeed if the tag matches. On free, the memory's tag is invalidated so any subsequent dereferences will fail and cause a panic.

This approach is inspired by the Vale language and ARM's Extended Memory Tagging Extension, which does memory tagging but accelerated at the hardware level. In the future when EMTE/MIE are brought to more platforms we should be able to remove the overhead of tagged pointers in the language and lean on that instead.

The general idea here is to decouple the concept of memory safety from a languages memory management technique. We can ensure memory safety by just checking memory is valid before using it, leaving us free to manage memory the way we want without fear of critical memory errors. Once that's in place we just need something to ensure that memory doesn't leak and that we don't have to think too much about it - like owned pointers (which are the same as c++'s `unique_ptr<T>` and Rust's `Box<T>`)

Example:
```void

const take_reference = fn(non_owned_int: *i32) {
  fmt.println("value: {:d}", non_owned_int.*)
}

const take_ownership = fn(owned_int: ^i32) {
  fmt.println("value: {:d}", owned_int.*)
 
  // since this function owns owned_int, it will be deallocated at the end of this scope
}

const main = fn() {
  // ^T signifies owning pointer - it has one owner and cannot be copied
  // owned values automatically deallocate at the close of their scope
  owned_int :^i32 = new(i32)

  // explicit "&" borrow syntax, even when passing pointer so the call site is obviously a potential mutation
  take_reference(&owned_int)

  // tagged pointer to our ^i32
  non_owned_int :*i32 = &owned_int

  // move the owned_int, invalidating the non_owned_int reference 
  take_ownership(&owned_int.move())

  // this will panic
  take_reference(&non_owned_int)
}
```

And a less contrived example:
```void
import json
import io

const User = struct {
  id: i64,
  name: ^string,
  hashed_password: ^string,
}

const User.new = fn(name: string, hashed_password: string) -> ^User {
  return new(User{
    .id = generate_next_id(), // imagine this function exists
    .name = string.new(name),
    .hashed_password = string.new(hashed_password),
  })
}

// like zig, !string means return an error OR a string
const User.to_json = fn(user: *User) -> !^string {
  // ! postfix operator means "propogate error up to caller", like rust's "?"
  return json.encode(&user)! 
}

const main = fn() -> ! {
  user := User.new("John Smith", "!@U$@DR3d2d")

  user_json := user.to_json()!
  fmt.println("user json: {}", user_json)

  output_file := io.File.open("output.txt", "w")!
  output_file.writeln(user_json)!

  // at close of scope user is deallocated, the file is closed, and the json string is deallocated
  // these will all still free correctly on an early return (like propogating an error)
}

```

This is all still in the theoretical stage, but I'm hopeful we can get something working with relatively similar ergonomics to a GC'd language

##### Escape Analysis
Commonly GC languages will analyse whether a piece of memory will need to "escape" to the heap – generally if a piece of memory is captured in a closure, returned, stored in a type with a longer lifetime, etc. In void, we start with owned `^T` and tagged `*T` pointers – owned pointers erase to raw pointers at runtime, while tagged pointers incur the runtime cost of validating the tag. There are cases however where a tagged pointer can skip tag checking – meaning at compile time we can guarantee that this piece of memory will not be free'd before usage.

A feature I'm planning is escape analyser – essentially a tiny, minimal borrow checker that attempts to validate memory lifetimes, and where it can safely do so, skip tag checks.

The philosophy here is to not obstruct the programmer, but allow them to optimise to the level of C performance.

Some examples:
* When a `^T` is passed to a function that accepts a `*T`, we automatically know that the memory will outlive the callee
* When a *T is only used at the same scope or in a scope with a longer lifetime as it's referent `^T`

Eventually I'd like to get to the point of memory management being similar to thinking about state in React – just move the owned memory to the shortest living scope that encloses all usages of that memory, and in doing so remove all runtime checks on memory before dereference.

#### Errors
void has an error type similar to Go's:

```void
const error = interface {
  message: fn(*Self)->string
}
```

Functions are marked as possibly erroring by making the return type an error union

```void
const cant_fail = fn() -> i32 do return 10

const can_fail = fn(condition: bool) -> !i32 {
  if condition {
    return 10 
  } else {
    return error.new("Condition cannot be false")
  }  
}
```

To check for errors you can:
```void

const main = fn() -> ! {
  // check explicitly
  value := can_fail()
  if value.is_err() {
    fmt.println("found an error: {}", value.err())
    return
  }

  // value is now considered unwrapped, since "is_err()" has been checked
  value += 10

  // propogate automatically

  unwrapped_value := can_fail()!
  unwrapped_value += 10

  // ! postfix will propogate errors up to the caller. When used in main it causes a panic, terminating the program
}
```
G