# ![XBLY](xbly.png)

**Syntactic sugar: XBLY (*zee-blee*) is (XML (But (Lispy)))**

XML is a powerful language with lots of tooling:
[schemas](https://en.wikipedia.org/wiki/XML_schema#Languages),
[XPath](https://en.wikipedia.org/wiki/XPath),
[XSLT](https://en.wikipedia.org/wiki/XSLT), validators, etc.

Unfortunately, it's a pain to read and write by hand. There's no inherent
reason why: it's only an accident of history that we're not currently
authoring structured documents with S-expressions and calling it the
[Common Business Communication Language](http://www-formal.stanford.edu/jmc/cbcl.html)
instead.

Enter XBLY. XBLY is a language that can be translated into XML
documents or XML fragments using an absurdly simple parser. We include an
implementation in C that you should have little trouble translating into
your language of choice.

The parser is so simple because it has no concept of a document - it just
transforms text using seven states, two bytes of context, and a small stack
for element names.

## Examples

### Basket

#### XBLY Input

```
<?xml version="1.0" encoding="utf-8"?>
(basket
    (item Apple)
    (item \quantity="2" Banana)
    (item Ginger \(fresh\))
)
```

#### XML Output

```
<?xml version="1.0" encoding="utf-8"?>
<basket>
    <item>Apple</item>
    <item quantity="2">Banana</item>
    <item>Ginger (fresh)</item>
</basket>
```

The XML output may not be binary identical to this example because the XML
output may contain extra whitespace inside tags. The result will be
semantically equivalent. Your XML process ends in XML Normalization or
Canonicalized XML anyway, right?

### XHTML Document

This example demonstrates some subtle differences in use of whitespace.

#### XBLY Input

```
<?xml version="1.0" ?>
<!DOCTYPE html>
(html \xmlns="http://www.w3.org/1999/xhtml"
    (head (title XHTML Example))
    (body
        (p Hello (b World))
        (hr \id="foo")
        (p Hello(u \ World ))
    )
)
```

#### XML Output

```
<?xml version="1.0" ?>
<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml">
    <head><title>XHTML Example</title></head>
    <body>
        <p>Hello <b>World</b></p>
        <hr id="foo"/>
        <p>Hello<u> World </u></p>
    </body>
</html>
```

Note that XHTML **must** be served as `application/xhtml+xml`, not as HTML.

If you want to generate HTML then you **must** transform the input once
more to meet [HTML Compatibility Guidelines](https://www.w3.org/TR/xhtml1/#C)
e.g. rewrite `<script src="..." />` to `<script src="..."></script>`, etc.
This can be done with XLST.

As a simple workaround, you can add some safe inner content instead, like a
JavaScript comment:

`(script \src="..." //)`


### Document Fragments

XBLY copies verbatim everything up to the first opening parenthesis, so we've
been able to include XML and HTML5 declarations. You can leave them out if you
want: they have no special meaning to XBLY.

#### XBLY Input

```
(hello (world))
```

#### XML Fragment Output

```
<hello><world /></hello>
```

### Escaping

Just to note about escaping with backslash:

In text content a backslash can escape explicit whitespace, the opening and
closing parenthesis, or a literal backslash.
 
You might think the same would work with quote character in attributes. Sorry,
but you have to use XML rules here and use `&quot;` or `&apos;`. It's
worth it to keep the parser simple and easy to translate to other langauges.

#### XBLY Input

```
(weather \what='it&apos;s raining men' \advice="Don't you lose your head!")
```

#### XML Fragment Output

```
<weather what='it&apos;s raining men' advice="Don't you lose your head!" />
```



## Try XBLY

We include a simple command-line program to convert a XBLY text stream to XML
documents or fragments.

**Clone the repo**

```
git clone git@github.com:tawesoft/xbly.git
cd xbly
```

**Compile xbly.c**

```
cc -std=c99 -Wall -Wextra xbly.c -o xbly
python3 test.py # optional
```

**Translate XBLY on the command-line**

```
echo "(foo (bar))" | ./xbly
cat input.xbly | ./xbly > output.xml
```

**Install XBLY**

```
sudo cp xbly /usr/local/xbly
```

## License

XBLY is licensed as [MIT-0 / MIT No Attribution](https://spdx.org/licenses/MIT-0.html)
so you can use it freely without credit. See COPYING.txt for the full terms.


