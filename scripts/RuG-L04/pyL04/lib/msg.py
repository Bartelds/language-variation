
from lang import lang

if lang == 'nl':
    from nl import text


if lang == 'en':

    def _(s):
        return s

else:

    def _(s):
        if text.has_key(s):
            t = text[s]
            if t:
                return t
        return s
