# luxem 0.0.1

## What is luxem?

luxem is a specification for serializing structured data.  

luxem is similar to JSON.  The main differences are:

1. You can specify a type using `(typename)` before any value. Ex: `(direction) up`.
2. You can have a `,` after the final element in an object or array.
3. Quotes are optional for simple strings (strings containing no spaces and no ambiguous symbols).
4. The document is an array with implicit (excluded) `[]` delimiters.
5. Comments (written as `*comment text*`) can be placed anywhere whitespace is.

All documents should be UTF-8 with 0x0A line endings (linux-style).

No basic types are defined in the parsing specification, but the following should be used as a guideline for minimum data type support:

* `bool` `true|false|yes|no|1|0`
* `int` `-?[0-9]+`
* `float` `-?[0-9]+(\.[0-9]+)?`
* `string`
* `ascii16` `([a-p][a-p])*`
* `null`

`ascii16` is a binary encoding that is both ugly and easy to parse, using the first 16 characters of the alphabet.  `bool` is not case-sensitive.  `null` should not necessarily be accepted in all contexts, but when a null value is required it should be specified as `null`.

## Why?

In general, I think JSON is excellent.  XML is bloated (header boiler-plate, number of reserved symbols) and inconsice, with multiple ways to represent the same data, and formatting can corrupt the data in the document.  YAML is bloated, fragile, and inconsistent, mixing human-readable text with arcane symbolic sequences.

However, several JSON use cases are very difficult or impossible:

- There is no way to serialize and deserialize polymorphic data.  

  An object key-value pair could be used to identify an object type, but, since objects elements are unordered, a validated document could have the type-key occur after the object body, preventing efficient streaming.

  An array can be used to guarantee the type identifier occurs before the body, but this is visually indistinguishable from other array uses and requires following a non-standard convention.

- Enumeration fields are indistinguishable from free-entry strings.

- There is native way to format binary data.

- Writing a full parser is unnecessarily difficult:

  The numeric notation in the form `4e10` has limited value and complicates the parser.  `null` must be accepted everywhere in the document, regardless of the data domain.  UTF-16 escapes must be understood and converted.

- There is no way to comment documents.

  Workarounds involving ignored strings cause a multitude of problems, including increasing the likelihood of key collisions in objects, difficulty to use in array contexts, and requiring in-application comment filtering.  

  Workarounds involving preprocessors to remove comments increase processing complexity, software dependencies, and (not always accessible) pipeline changes, and still result in non-standard JSON files.

## Implementations

- [luxem-c](https://github.com/Rendaw/luxem-c)

  This is a barebones C implementation, with no buffer allocation, type translation, or structure generation support.

- [luxem-python](https://github.com/Rendaw/luxem-python)

  The Python implementation wraps luxem-c and adds support for type translations and structure generation.

- [luxem-cxx](https://github.com/Rendaw/luxem-cxx)

  This is roughly equivalent to the Python version, with a C++ boxed-type implementation for building structures.

## Tools

- [luxemx](https://github.com/Rendaw/luxemx)

  `luxemx` is a command-line tool for extracting elements from luxem documents.  A poor-man's luxem jq.

- [luxemog](https://github.com/Rendaw/luxemog)

  `luxemog` reads rules from a file and uses them to transform a luxem document.

## Cool Tricks

### Specifying a file type

```luxem
(favnum v1.1.0) [
	2, 17, 11, 9, 2, 23
]
```

### Merging sample sets

```luxem
{x: 7, y: 3, weight: 1.29867},
{x: 1, y: -20, weight: 0.24234},
```

+

```luxem
{x: -19, y: -22, weight: 0.33011},
```

 =

```luxem
{x: 7, y: 3, weight: 1.29867},
{x: 1, y: -20, weight: 0.24234},
{x: -19, y: -22, weight: 0.33011},
```

All three of the above are valid documents.

### Minimalism

```luxem
9, 2
```

## Almost Formal Specification
```
<START-HERE> ::= <w> <array-body> <w>

<array> ::= "[" <w> <array-body> <w> "]"

<array-body> ::= [<value-phrase> <w> <array-elements>]

<array-elements> ::= "," <w> <value-phrase>
	| ","

<value-phrase> ::= [<type> <w>] <value>

<type> ::= "(" <words-not-)> ")"

<value> ::= <primitive>
	| <object>
	| <array>

<primitive> ::= <string>

<string> ::= <word>
	| "\"" <words-not-"> "\""

<object> ::= "{" <w> <object-body> <w> "}"

<object-body> ::= [<object-element-phrase> <w> <object-elements>]

<object-elements> ::= "," <w> <object-element-body> <w> <object-elements>
	| ","

<object-element-prhase> ::= <key> <w> ":" <w> <value-phrase>

<w> ::= [" "] ["\n"] ["\t"] ["*" <words-not-*>  "*"] [<w>]

```
`<word>` matches `[^ ([,:"*]*`.

`<words-not-*>` matches a sequence of characters excluding the non-escaped delimiter (represented as `*` here).  Characters can be escaped using `\`.

All documents should be UTF-8 with 0x0A line endings (linux-style).

The name `luxem` is always lowercase.

---
&copy; Rendaw, Zarbosoft 2014

