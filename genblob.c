#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int
usage(const char *name)
{
    fprintf(stderr, ""
"Usage: %s [OPTION]... FILE\n"
"Generate *.c and *.h constant array file from an arbitrary text file.\n"
"\n"
"Options:\n"
"  -i FILE.h  header file to generate\n"
"  -o FILE.c  source file to generate\n"
"  -s SYMBOL  C symbol name\n"
            "", name);
    return 1;
}

int
main(int argc, char **argv)
{
    const char *symbol = NULL;
    const char *input = NULL;
    const char *header = NULL;
    const char *source = NULL;

    {
        int opt;
        while ((opt = getopt(argc, argv, "s:i:o:h")) != -1) {
            switch (opt) {
                case 's':
                    if (symbol)
                        return usage(argv[0]);
                    symbol = optarg;
                    break;
                case 'i':
                    if (header)
                        return usage(argv[0]);
                    header = optarg;
                    break;
                case 'o':
                    if (source)
                        return usage(argv[0]);
                    source = optarg;
                    break;
                case 'h':
                    return usage(argv[0]);
                default:
                    return usage(argv[0]);
            }
        }

        if (optind != argc - 1)
            return usage(argv[0]);
        input = argv[optind];
    }

    if (! symbol)
        symbol = "blob";

    if (! symbol || ! header || ! source)
        return usage(argv[0]);

    int ret = 1;

    {
        FILE *in = NULL;
        FILE *src = NULL;
        FILE *hdr = NULL;
        const char *ename;
        const char *eop = "open";

        in = fopen((ename = input), "rb");
        if (! in)
            goto error;

        hdr = fopen((ename = header), "w");
        if (! hdr)
            goto error;

        src = fopen((ename = source), "w");
        if (! src)
            goto error;

        eop = "write";
        ename = source;
        if (fprintf(src, "/* GENERATED FILE - DO NOT EDIT */\nconst char %s[] = {", symbol) < 0)
            goto error;

        unsigned size = 0;


        while (! feof(in)) {
            char buf[1024];
            eop = "read";
            ename = input;
            ssize_t rd = fread(buf, 1, sizeof(buf), in);
            if (rd < 0)
                goto error;
            eop = "write";
            ename = source;
            for (unsigned i = 0; i < rd; ++i) {
                const char *brk = ((size + i) % 8) ? "" : "\n   ";
                if (fprintf(src, "%s 0x%02x,", brk, (unsigned char)buf[i]) < 0)
                    goto error;
            }
            size += rd;
        }

        eop = "write";
        ename = source;
        if (fprintf(src, "\n};\n") < 0)
            goto error;
        ename = header;
        if (fprintf(hdr, "extern const char %s[%u];\n", symbol, size) < 0)
            goto error;

        ret = 0;
        goto close;

    error:
        fprintf(stderr, "genblob: unable to %s `%s': %s\n", eop, ename, strerror(errno));
    close:
        if (src)
            fclose(src);
        if (hdr)
            fclose(hdr);
        if (in)
            fclose(in);
    }

    return ret;
}
