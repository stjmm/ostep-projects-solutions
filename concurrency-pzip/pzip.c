#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "common_threads.h"

static void die(const char *msg)
{
    perror(msg);
    exit(1);
}

typedef struct _file_view {
    char *data;
    size_t size;
    int owns_data; // 1 if malloc'd, 0 if mmap'd
} file_view;

static file_view load_files(int argc, char **argv)
{
    size_t total = 0;
    for (int i = 1; i < argc; i++) {
        struct stat st;
        if (stat(argv[i], &st) < 0)
            die(argv[i]);
        total += st.st_size;
    }
    if (total == 0)
        return (file_view){ NULL, 0, 0 };

    if (argc == 2) {
        int fd = open(argv[1], O_RDONLY);
        if (fd < 0)
            die(argv[1]);

        char *p = mmap(NULL, total, PROT_READ, MAP_PRIVATE, fd, 0);
        if (p == MAP_FAILED)
            die(argv[1]);

        close(fd);
        madvise(p, total, MADV_SEQUENTIAL);
        return (file_view){ p, total, 0 };
    }

    char *buf = malloc(total);
    if (!buf) die("malloc");
    char *dst = buf;
    for (int i = 1; i < argc; i++) {
        FILE *f = fopen(argv[i], "r");
        if (!f)
            die(argv[i]);

        struct stat st;
        if (stat(argv[i], &st) < 0)
            die(argv[i]);

        size_t n = fread(dst, 1, (size_t)st.st_size, f);
        if (n != (size_t)st.st_size)
            die("fread");
        fclose(f);
        dst += n;
    }

    return (file_view){ buf, total, 1 };
}

static void free_view(file_view *view)
{
    if (view->owns_data)
        free(view->data);
    else
        munmap(view->data, view->size);
}
typedef struct __run {
    uint32_t count;
    char c;
} run;

typedef struct __chunk_result {
    run *runs;
    size_t nruns;
    size_t cap;
} chunk_result;

static void result_push(chunk_result *r, uint32_t count, char c)
{
    if (r->nruns == r->cap) {
        r->cap = r->cap ? r->cap * 2 : 256;
        r->runs = realloc(r->runs, r->cap * sizeof(run));
        if (!r->runs)
            die("realloc");
    }
    r->runs[r->nruns++] = (run){ count, c };
}

typedef struct __worker_arg {
    const char *start_addr;
    size_t size;
    chunk_result result;
} worker_arg;

static void *worker(void *arg)
{
    worker_arg *w = (worker_arg*)arg;
    const char *p = w->start_addr;
    const char *end = p + w->size;
    chunk_result *r = &w->result;
    memset(r, 0, sizeof(*r));

    if (p == end) return NULL;

    char cur = *p;
    uint32_t count = 1;
    p++;

    while (p < end) {
        if (*p == cur) {
            count++;
            if (count == UINT32_MAX) {
                result_push(r, count, cur);
                count = 0;
            }
        } else {
            result_push(&w->result, count, cur);
            cur = *p;
            count = 1;
        }
        p++;
    }

    result_push(&w->result, count, cur);
    return NULL;
}

#define OUT_BUF_SIZE 64 * 1024

typedef struct __out_buf {
    char buf[OUT_BUF_SIZE];
    size_t pos;
} out_buf;

static void out_flush(out_buf *ob)
{
    if (ob->pos == 0) return;
    if (fwrite(ob->buf, 1, ob->pos, stdout) != ob->pos)
        die("fwrite");
    ob->pos = 0;
}

static void out_run(out_buf *ob, uint32_t count, char c)
{
    if (ob->pos + 5 > OUT_BUF_SIZE) out_flush(ob);
    ob->buf[ob->pos++] = (char)(count & 0xFF);
    ob->buf[ob->pos++] = (char)(count >> 8 & 0xFF);
    ob->buf[ob->pos++] = (char)(count >> 16 & 0xFF);
    ob->buf[ob->pos++] = (char)(count >> 24 & 0xFF);
    ob->buf[ob->pos++] = c;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file1 [file2 ...]\n", argv[0]);
        exit(1);
    }

    file_view fv = load_files(argc, argv);

    int numprocs = get_nprocs();

    if (numprocs >= fv.size)
        numprocs = (int)fv.size;

    worker_arg *workers = calloc((size_t)numprocs, sizeof(worker_arg));
    if (!workers)
        die("calloc");

    size_t chunk = fv.size / numprocs;
    size_t rem = fv.size % numprocs;
    const char *cur = fv.data;

    for (int i = 0; i < numprocs; i++) {
        workers[i].start_addr = cur;
        workers[i].size = chunk + (i == numprocs - 1 ? rem : 0);
        cur += workers[i].size;
    }

    pthread_t *threads = malloc(sizeof(pthread_t) * numprocs);
    if (!threads)
        die("malloc");

    for (int i = 0; i < numprocs; i++)
        Pthread_create(&threads[i], NULL, worker, &workers[i]);

    for (int i = 0; i < numprocs; i++)
        Pthread_join(threads[i], NULL);

    // Merge and write output
    out_buf ob = {0};

    char cur_c = 0;
    uint32_t cur_count = 0;
    int first = 1;

    for (int i = 0; i < numprocs; i++) {
        chunk_result *r = &workers[i].result;
        for (size_t j = 0; j < r->nruns; j++) {
            run *rn = &r->runs[j];
            if (first) {
                cur_c = rn->c;
                cur_count = rn->count;
                first = 0;
            } else if (rn->c == cur_c) {
                cur_count += rn->count;
            } else {
                out_run(&ob, cur_count, cur_c);
                cur_c = rn->c;
                cur_count = rn->count;
            }
        }
    }
    if (!first)
        out_run(&ob, cur_count, cur_c);

    out_flush(&ob);

    free(threads);
    free(workers);
    free_view(&fv);

    return 0;
}
