# luxem 0.1

## What is luxem?

luxem is a specification for serializing structured data.  

luxem is similar to JSON.  The main differences are:

1. You can specify a type using `(typename)` before any value. Ex: `(direction) up`.
2. You can have a `,` after the final element in an object or array.
3. Quotes are optional for simple strings (strings containing no spaces and no ambiguous symbols).
4. The document is an array with implicit (excluded) `[]` delimiters.

All documents should be UTF-8 with 0x0A line endings (linux-style).

No basic types are defined in the parsing specification, but the following should be used as a guideline for minimum data type support:

* `bool` `true|false|yes|no|1|0`
* `int` `-?[0-9]+`
* `float` `-?[0-9]+(\.[0-9]+)?`
* `string`
* `ascii16` `([a-p][a-p])*`

`ascii16` is a binary encoding that is both ugly and easy to parse, using the first 16 characters of the alphabet.  `bool` is not case-sensitive.

## Why?

In general, I think JSON is excellent.  XML is bloated (header boiler-plate, number of reserved symbols) and inconsice, with multiple ways to represent the same data, and formatting can corrupt the data in the document.  YAML is bloated, fragile, and inconsistent, mixing human-readable text with arcane symbolic sequences.

However, several JSON use cases are very difficult or impossible:

1. There is no way to identify types in a mixed-type structure (for example, when serializing polymorphic objects).  

  An object key-value pair could be used to identify an object type, but, since objects elements are unordered, a validated document could have the type-key occur after the object body, preventing efficient streaming.

  An array can be used to guarantee the type identifier occurs before the body, but this is visually indistinguishable from other array uses and requires following a non-standard convention.

2. The existing primitive types are too restrictive. 

  There is no enumeration type.

  There is no type for raw binary data.

3. The existing primitive types are too complex.

  The numeric notation in the form `4e10` is unnecessary and complicates the parser.  `null` must be accepted everywhere in the document, regardless of the domain.  Unicode escapes are specified as UTF-16, and must be understood (and in some cases translated) by a compliant parser.

  Standard types, such as numbers and boolean values are unambiguously differentiated, but differentiating them is rarely important - you expect one or another type by context.  In situations where you do need to distinguish types, there is no guarantee (or likeliness) that your division will fall into the type categories provided by JSON.

3. The document structure is overspecified.

  Many uses of JSON involve a document that is a list.  To represent this in conformant JSON, you need a minimum of an object, a key, and an array (*update:* This requirement has been relaxed in a recent specification and is no longer true).  

  You cannot concatenate documents without a full JSON parser.

## Implementations

- [luxem-c](https://github.com/Rendaw/luxem-c)

  This is a barebones C implementation, with no buffer allocation, type translation, or structure generation support.

- [luxem-python](https://github.com/Rendaw/luxem-python)

  The Python implementation wraps luxem-c and adds support for type translations and structure generation.

- [luxem-cxx](https://github.com/Rendaw/luxem-cxx)

  This is roughly equivalent to the Python version, with a C++ boxed-type implementation for building structures.

## Tools

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

```
`<w>` matches any whitespace character.

`<word>` matches `[^ ([,:"]`.

`<words-not-*>` matches a sequence of characters excluding the non-escaped delimiter (represented as `*` here).  Characters can be escaped using `\`.

All documents should be UTF-8 with 0x0A line endings (linux-style).

