extern int end;                   // 这个 end 符号是由编译器生成的，整个 system 模块的最后一个地址，描述不准确，可以查看 .map 文件找这个标号

struct buffer_head * start_buffer = (struct buffer_head *) &end;

void buffer_init(long buffer_end) {

}

