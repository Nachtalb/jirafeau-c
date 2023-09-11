autocmd BufWritePost main.c !cd build && make
autocmd BufWritePost CMakeLists.txt !cd build && cmake ..
autocmd BufWritePost *.c,*.h !uncrustify -c .uncrustify.cfg % --replace --no-backup
