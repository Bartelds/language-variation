
import re
import sys

# default language
lang = 'en'

# Windows language IDs
_id = {
    0x0409: 'English',
    0x040c: 'French',
    0x0c0a: 'Spanish',
    0x0410: 'Italian',
    0x041D: 'Swedish',
    0x0413: 'Dutch',
    0x0416: 'Brazilian',
    0x040b: 'Finnish',
    0x0414: 'Norwegian',
    0x0406: 'Danish',
    0x040e: 'Hungarian',
    0x0415: 'Polish',
    0x0419: 'Russian',
    0x0405: 'Czech',
    0x0408: 'Greek',
    0x0816: 'Portuguese',
    0x041f: 'Turkish',
    0x0411: 'Japanese',
    0x0412: 'Korean',
    0x0407: 'German',
    0x0804: 'Chinese (Simplified)',
    0x0404: 'Chinese (Traditional)',
    0x0401: 'Arabic',
    0x040d: 'Hebrew',
    }

_lang = ''
if sys.platform[:3] == 'win':
    try:
        from _winreg import *
        _k = OpenKey(HKEY_CURRENT_USER, "Control Panel\\International")
        _l = QueryValueEx(_k, "Locale")[0]
        _k.Close()
        _i = int(_l, 16)
        if _id.has_key(_i):
            _lang = _id[_i]
        else:
            _k = OpenKey(HKEY_LOCAL_MACHINE,
                         "System\\CurrentControlSet\\Control\\Nls\\Locale")
            _lang = QueryValueEx(_k, _l)[0]
            _k.Close()
    except:
        _lang = ''
else:
    import os
    _lang = os.getenv('LANG', '')

# supported languages only!
if re.match('nl|dutch', _lang, re.I):
    lang = 'nl'

if __name__ == '__main__':
    print lang
