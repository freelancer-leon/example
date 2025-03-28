unsigned long foo = 0xdeadbeef;

int main() {
    void (*func)() = (void (*)())&foo;
    func();
    return 0;
}
