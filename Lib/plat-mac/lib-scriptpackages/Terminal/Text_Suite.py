"""Suite Text Suite: A set of basic classes for text processing.
Level 1, version 1

Generated from /Applications/Utilities/Terminal.app
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = '????'

class Text_Suite_Events:

	pass


class attachment(aetools.ComponentItem):
	"""attachment - Represents an inline text attachment.  This class is used mainly for make commands. """
	want = 'atts'
class _3c_Inheritance_3e_(aetools.NProperty):
	"""<Inheritance> - All of the properties of the superclass. """
	which = 'c@#^'
	want = 'ctxt'
class file_name(aetools.NProperty):
	"""file name - The path to the file for the attachment """
	which = 'atfn'
	want = 'utxt'
#        element 'catr' as ['indx', 'rele', 'rang', 'test']
#        element 'cha ' as ['indx', 'rele', 'rang', 'test']
#        element 'cpar' as ['indx', 'rele', 'rang', 'test']
#        element 'cwor' as ['indx', 'rele', 'rang', 'test']

class attribute_run(aetools.ComponentItem):
	"""attribute run - This subdivides the text into chunks that all have the same attributes. """
	want = 'catr'
class color(aetools.NProperty):
	"""color - The color of the first character. """
	which = 'colr'
	want = 'colr'
class font(aetools.NProperty):
	"""font - The name of the font of the first character. """
	which = 'font'
	want = 'utxt'
class size(aetools.NProperty):
	"""size - The size in points of the first character. """
	which = 'ptsz'
	want = 'long'
#        element 'catr' as ['indx', 'rele', 'rang', 'test']
#        element 'cha ' as ['indx', 'rele', 'rang', 'test']
#        element 'cpar' as ['indx', 'rele', 'rang', 'test']
#        element 'cwor' as ['indx', 'rele', 'rang', 'test']

attribute_runs = attribute_run

class character(aetools.ComponentItem):
	"""character - This subdivides the text into characters. """
	want = 'cha '
#        element 'catr' as ['indx', 'rele', 'rang', 'test']
#        element 'cha ' as ['indx', 'rele', 'rang', 'test']
#        element 'cpar' as ['indx', 'rele', 'rang', 'test']
#        element 'cwor' as ['indx', 'rele', 'rang', 'test']

characters = character

class paragraph(aetools.ComponentItem):
	"""paragraph - This subdivides the text into paragraphs. """
	want = 'cpar'
#        element 'catr' as ['indx', 'rele', 'rang', 'test']
#        element 'cha ' as ['indx', 'rele', 'rang', 'test']
#        element 'cpar' as ['indx', 'rele', 'rang', 'test']
#        element 'cwor' as ['indx', 'rele', 'rang', 'test']

paragraphs = paragraph

class text(aetools.ComponentItem):
	"""text - Rich (styled) text """
	want = 'ctxt'
#        element 'catr' as ['indx', 'rele', 'rang', 'test']
#        element 'cha ' as ['indx', 'rele', 'rang', 'test']
#        element 'cpar' as ['indx', 'rele', 'rang', 'test']
#        element 'cwor' as ['indx', 'rele', 'rang', 'test']

class word(aetools.ComponentItem):
	"""word - This subdivides the text into words. """
	want = 'cwor'
#        element 'catr' as ['indx', 'rele', 'rang', 'test']
#        element 'cha ' as ['indx', 'rele', 'rang', 'test']
#        element 'cpar' as ['indx', 'rele', 'rang', 'test']
#        element 'cwor' as ['indx', 'rele', 'rang', 'test']

words = word
attachment._superclassnames = ['text']
attachment._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'file_name' : file_name,
}
attachment._privelemdict = {
	'attribute_run' : attribute_run,
	'character' : character,
	'paragraph' : paragraph,
	'word' : word,
}
import Standard_Suite
attribute_run._superclassnames = ['item']
attribute_run._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'color' : color,
	'font' : font,
	'size' : size,
}
attribute_run._privelemdict = {
	'attribute_run' : attribute_run,
	'character' : character,
	'paragraph' : paragraph,
	'word' : word,
}
character._superclassnames = ['item']
character._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'color' : color,
	'font' : font,
	'size' : size,
}
character._privelemdict = {
	'attribute_run' : attribute_run,
	'character' : character,
	'paragraph' : paragraph,
	'word' : word,
}
paragraph._superclassnames = ['item']
paragraph._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'color' : color,
	'font' : font,
	'size' : size,
}
paragraph._privelemdict = {
	'attribute_run' : attribute_run,
	'character' : character,
	'paragraph' : paragraph,
	'word' : word,
}
text._superclassnames = ['item']
text._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'color' : color,
	'font' : font,
	'size' : size,
}
text._privelemdict = {
	'attribute_run' : attribute_run,
	'character' : character,
	'paragraph' : paragraph,
	'word' : word,
}
word._superclassnames = ['item']
word._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'color' : color,
	'font' : font,
	'size' : size,
}
word._privelemdict = {
	'attribute_run' : attribute_run,
	'character' : character,
	'paragraph' : paragraph,
	'word' : word,
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'atts' : attachment,
	'catr' : attribute_run,
	'cha ' : character,
	'cpar' : paragraph,
	'ctxt' : text,
	'cwor' : word,
}

_propdeclarations = {
	'atfn' : file_name,
	'c@#^' : _3c_Inheritance_3e_,
	'colr' : color,
	'font' : font,
	'ptsz' : size,
}

_compdeclarations = {
}

_enumdeclarations = {
}
