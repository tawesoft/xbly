
from subprocess import Popen, PIPE
from xml.dom.minidom import parseString

def NormaliseXML(source):
    try:
        return parseString(source).toxml()
    except Exception as e:
        return str(e)

def Translate(source):
    p = Popen(['./xbly'], stdout=PIPE, stdin=PIPE, stderr=PIPE)
    outs, errs = p.communicate(input=source.encode())

    p.stdout.close()
    p.stdin.close()
    p.stderr.close()
    p.kill()

    return outs.decode(), errs.decode()

# Each test is a 2-tuple (XBLY input, expected XML output). XML output is
# normalised before being compared.
#
# Don't include tests where XBLY accepts invalid XML - that's not XBLYs fault
# if Garbage In = Garbage Out!
tests = [
    (
        '''(hello)''',
        '''<?xml version="1.0" ?><hello/>'''
    ),

    (
        '''<?xml version="1.0" ?>(hello)''',
        '''<?xml version="1.0" ?><hello/>'''
    ),

    (
    '''<?xml version="1.0" ?>
<!DOCTYPE html>
(html \\xmlns="http://www.w3.org/1999/xhtml"
    (head (title XHTML Example))
    (body
        (p Hello (b World))
        (hr \id="foo")
        (p Hello(u \\ World ))
    )
)''',
        '''<?xml version="1.0" ?><!DOCTYPE html><html xmlns="http://www.w3.org/1999/xhtml"><head><title>XHTML Example</title></head>
    <body><p>Hello <b>World</b></p>
        <hr id="foo"/>
        <p>Hello<u> World </u></p>
    </body>
</html>''',
    ),

    (
        '''<?xml version="1.0" encoding="utf-8"?>
(basket
    (item Apple)
    (item \quantity="2" Banana)
    (item Ginger \(fresh\))
)''',
        '''<?xml version="1.0" ?><basket><item>Apple</item>
    <item quantity="2">Banana</item>
    <item>Ginger (fresh)</item>
</basket>''',
    ),
]

for source, expected in tests:

    out, errs = Translate(source)
    if errs:
        print("Test failed (XBLY error)")
        print("Input: "+source)
        print("Output: "+out)
        print("Errors: "+errs)

    normalised = NormaliseXML(out)
    if normalised != expected:
        print("Test failed (output not as expected)")
        print("Input: "+source)
        print("Output: "+out)
        print("Output (normalised): "+normalised)
        print("Expected: "+expected)



