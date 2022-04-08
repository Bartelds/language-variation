
class Help:

    def __init__(self, text):
        self.text = text.strip('\n\r')

    def __call__(self):
        print
        print '=' * 72
        print self.text
        print '-' * 72
        print
