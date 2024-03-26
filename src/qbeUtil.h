#pragma once

#include <cassert>
#include <cstdio>
#include <optional>
#include <span>
#include <string>


namespace qbec {

extern "C" {
    #define export _export
    #include <qbe/all.h>
    #undef export

    extern Target T_amd64_sysv;
}

} // namespace qbec


namespace qbe {


static FILE *outf;

static void dbgfile(char *fn) {
    qbec::emitdbgfile(fn, outf);
}

static void data(qbec::Dat *d) {
    emitdat(d, outf);
    if (d->type == qbec::Dat::DEnd) {
        fputs("/* end data */\n\n", outf);
        qbec::freeall();
    }
}

static void func(qbec::Fn *fn) {
    uint n;

    qbec::T.abi0(fn);
    qbec::fillrpo(fn);
    qbec::fillpreds(fn);
    qbec::filluse(fn);
    qbec::promote(fn);
    qbec::filluse(fn);
    qbec::ssa(fn);
    qbec::filluse(fn);
    qbec::ssacheck(fn);
    qbec::fillalias(fn);
    qbec::loadopt(fn);
    qbec::filluse(fn);
    qbec::fillalias(fn);
    qbec::coalesce(fn);
    qbec::filluse(fn);
    qbec::ssacheck(fn);
    qbec::copy(fn);
    qbec::filluse(fn);
    qbec::fold(fn);
    qbec::T.abi1(fn);
    qbec::simpl(fn);
    qbec::fillpreds(fn);
    qbec::filluse(fn);
    qbec::T.isel(fn);
    qbec::fillrpo(fn);
    qbec::filllive(fn);
    qbec::fillloop(fn);
    qbec::fillcost(fn);
    qbec::spill(fn);
    qbec::rega(fn);
    qbec::fillrpo(fn);
    qbec::simpljmp(fn);
    qbec::fillpreds(fn);
    qbec::fillrpo(fn);
    assert(fn->rpo[0] == fn->start);
    for (n=0;; n++) {
        if (n == fn->nblk-1) {
            fn->rpo[n]->link = 0;
            break;
        }
        else {
            fn->rpo[n]->link = fn->rpo[n+1];
        }
    }
    qbec::T.emitfn(fn, outf);
    fprintf(outf, "/* end function %s */\n\n", fn->name);

    qbec::freeall();
}


inline std::optional<std::string> compile(std::string path, std::span<char> code, bool emitfin) {
    qbec::T = qbec::T_amd64_sysv;

    FILE* f = fmemopen(static_cast<void*>(code.data()), code.size(), "r");
    if (!f) {
        return std::nullopt;
    }

    char* outBuf;
    size_t outSize;
    outf = open_memstream(&outBuf, &outSize);

    qbec::parse(f, path.data(), dbgfile, data, func);

    if (emitfin) {
        qbec::T.emitfin(outf);
    }

    fputc('\0', outf);

    fclose(f);
    fclose(outf);

    std::string as(outBuf, outSize);

    free(outBuf); // NOLINT

    return as;
}


} // namespace qbe
