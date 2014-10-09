local sources = tup.glob('*.c')
tup.definerule{
	inputs = sources,
	outputs = {'luxem.so'},
	command = 'clang -ansi -pedantic -Wall -Werror -ggdb -O0 -fPIC -shared -o luxem.so ' .. table.concat(sources, ' ')
}
