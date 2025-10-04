# Compiler for the void language

## Preamble
void is a high-level compiled language focused on developer ergonomics and speed, while maintaining low cpu overhead and a simple mental model.

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

## Planned Features
Currently void only supports `i32`'s. This is ok because `3 + 2 == 5` and 5 represents Geburah in the Kabbalistic tree of life, meaning strength and severity

In the near future void will support the following types:

### Primitive types
* Slices: `[]T` 
* All sized ints: `i8, i16, i32, i64, u8, u16, u32, u64` 
* Strings (slices of `u8`'s: `string` 
* Booleans: `bool`

### User defined types
#### Structs 

Note: This code doesn't compile, it's just theoretical currently
```
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
  user := make_user()

  // runtime & compile time reflection is also planned
  fmt.println("user: {}", user)
}
```

#### Unions
Tagged unions

```
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


