#include "./code_sequence.h"
#include <string>
#include <algorithm>
#include <iostream>
#include "./exception.h"
#include "./unicode_data.h"
#include "./normalizer.h"

using std::string;
using std::vector;

namespace langdetect {


CodeSequence::CodeSequence(char const *data, size_t const &length) {
    string buf(data, length);
    cleanchar_(buf);
    size_t cursor(0);
    while(cursor < string::npos) {
        uint32_t code = readchar_(buf, cursor);
        if(cursor == string::npos && code == 0) break;  // no more data
        codes_.push_back(code);
    }
    rough_judge_();
    Normalizer& normalizer = Normalizer::instance();
    normalizer.normalize(codes_);
}

vector<string> CodeSequence::tongram() {
    vector<string> ngrams;
    string ngram;
    for(size_t i = 0; i < codes_.size(); ++i) {
        ngram += string(reinterpret_cast<char *>(&codes_[i]), 4);
        if(ngram.length() > 12) ngram.erase(ngram.begin());
        if(!ngram.empty()) ngrams.push_back(ngram);
    }
    return ngrams;
}

uint32_t CodeSequence::readchar_(string const &data, size_t &cursor) {
    if(data.length() == 0) throw DetectError("empty data");
    if(cursor >= data.length()) throw DetectError("scan error");
    size_t clen(0);
    uint32_t code = 0 | data[cursor];
    if((data[cursor] & 0x80) == 0) clen = 1;
    else if((data[cursor] & 0xe0) == 0xc0) {
        clen = 2;
        code &= 0x1f;
    } else if((data[cursor] & 0xf0) == 0xe0) {
        clen = 3;
        code &= 0xf;
    } else if((data[cursor] & 0xfc) == 0xf0) {
        clen = 4;
        code &= 0x7;
    } else {
        throw DetectError("can't encode string: may not utf-8 encoding");
    }
    if(cursor + clen > data.length()) {  // 中途半端なバイト列が末尾にある。例外は投げないで、失敗を伝える。
        cursor = string::npos;
        return 0;
    }
    for(size_t i = 1; i < clen; ++i) {
        code = (code << 6) | (data[cursor + i] & 0x3f);
    }
    cursor += clen;
    if(cursor == data.length()) cursor = string::npos;  // finish sign
    return code;
}

void CodeSequence::cleanchar_(string &data) {
    string alnum = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    // URL_REGEX = Pattern.compile("https?://[-_.?&~;+=/#0-9A-Za-z]{1,2076}");
    // clean html
    size_t cursor(0);
    while(true) {
        size_t p = data.find("://", cursor);
        if(p == string::npos) break;
        cursor = p + 3;  // 無限ループしないようにはじめに進める。あとで必要なら修正する
        if(p < 3) continue;
        bool isurl = false;
        size_t start(0);
        if(data.compare(p - 3, 3, "ftp") == 0) {
            isurl = true;
            start = p - 3;
        } else if(p >= 4) {
            if(data.compare(p - 4, 4, "http") == 0 ||
                    data.compare(p - 4, 4, "file") == 0) {
                isurl = true;
                start = p - 4;
                if(p >= 5) {
                    if(data.compare(p - 5, 5, "https") == 0) {
                        isurl = true;
                        start = p - 5;
                    }
                }
            }
            if(isurl) {
                // 終わりを探す
                size_t end = data.find_first_not_of("-_.?&~;+=/#" + alnum);
                if(end - start <= 2076) {
                    data.replace(start, end - start, " ");
                    cursor = start + 1;
                }
            }
        }
    }
    // clean mail
    // MAIL_REGEX = Pattern.compile("[-_.0-9A-Za-z]{1,64}@[-_0-9A-Za-z]{1,255}[-_.0-9A-Za-z]{1,255}");
    cursor = 0;
    while(true) {
        size_t p = data.find("@", cursor);
        if(p == string::npos) break;
        cursor = p + 1;
        if(p == 0) continue;
        size_t start = data.find_last_not_of("-_." + alnum, p - 1);
        size_t end = data.find_last_not_of("-_." + alnum, p + 1);
        if(p - start <= 64 && end - p <= 510) {
            data.replace(start, end - start, " ");
            cursor = start + 1;
        }
    }
}

    void CodeSequence::rough_judge_() {
        int islatin(0), isnonlatin(0);
        for(size_t i = 0; i < codes_.size(); ++i) {
            uint32_t const &code = codes_[i];
            if(0x0041 <= code && code <= 0x007a) ++islatin;
            else if(code >= 0x0300 && UnicodeData::unicodeblock(code) != UNICODE_BLOCK_LATIN_EXTENDED_ADDITIONAL) {
                ++isnonlatin;
            }
        }
        if(islatin * 2 < isnonlatin) {
            vector<uint32_t> newcodes;
            for(size_t i = 0; i < codes_.size(); ++i) {
                uint32_t const &code = codes_[i];
                if(code > 0x007a || code < 0x0041) newcodes.push_back(code);
            }
            codes_ = newcodes;
        }
    }
}
