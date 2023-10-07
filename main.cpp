#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <map>
#include <set>

using namespace std;

//#define DEBUG

int main(int argc, char* argv[]) {
    //https://en.wikipedia.org/wiki/Vehicle_registration_plates_of_Singapore#Checksum
    auto get_check_sum_fn = [](const string& s) -> char {
        int len = (int)s.size();
        if (len < 3 || len > 7) return '\0';

        static int coefs[] = {9, 4, 5, 4, 3, 2};
        static const char* table = "AZYXUTSRPMLKJHGEDCB";

        int first_digit_pos = -1;
        for (int i = 0; i < len; ++i) {
            char c = s[i];
            if (-1 == first_digit_pos) {
                if (c >= '0' && c <= '9') first_digit_pos = i;
                else if (!(c >= 'A' && c <= 'Z')) return '\0';
            } else if (!(c >= '0' && c <= '9')) return '\0';
        }
        if (0 == first_digit_pos || first_digit_pos > 3 || len - first_digit_pos > 4) return '\0';

        int sum = 0;
        for (int i = first_digit_pos - 1, coefs_idx = 1; i >= 0 && coefs_idx >= 0; --i, --coefs_idx) {
            sum += coefs[coefs_idx] * (s[i] - 'A' + 1);
        }
        for (int i = len - 1, coefs_idx = 5; i >= first_digit_pos; --i, --coefs_idx) {
            sum += coefs[coefs_idx] * (s[i] - '0');
        }
        return table[sum % 19];
    };

    auto validate_fn = [&](const string& s) -> bool {
        char cs = get_check_sum_fn(s.substr(0, s.size() - 1));
#ifdef DEBUG
        printf("DEBUG %s -> %s%c\n", s.data(), s.substr(0, s.size() - 1).data(), cs);
#endif
        return s.back() == cs;
    };

    assert(validate_fn("E23H"));
    assert(validate_fn("SG2017C"));
    assert(validate_fn("SBS9889U"));

    auto try_replace_one_char_fn = [&](const string& s) -> vector<string> {
        assert(s.size() > 2);
        vector<string> res = {s};
        string t = s;
        for (int i = (s[2] >= 'A' && s[2] <= 'Z'); i < (int)s.size() - 1; ++i) {
            const char c = t[i];
            auto helper_fn = [&](char new_c) {
                if (c != new_c) {
                    t[i] = new_c;
                    if (validate_fn(t)) res.push_back(t);
                    t[i] = c;
                }
            };
            if (c >= '0' && c <= '9') {
                for (char j = '0'; j <= '9'; ++j) helper_fn(j);
            } else {
                for (char j = 'A'; j <= 'Z'; ++j) helper_fn(j);
            }
        }
        return res;
    };

    assert(1 == try_replace_one_char_fn("SMN1622X").size());
    assert(1 == try_replace_one_char_fn("SMR2630U").size());
    {
        const auto& res = try_replace_one_char_fn("SMY4387Y");
        assert(2 == res.size() && "SMF4387Y" == res[1]);
        /*
        for (int i = 1; i < (int)res.size(); ++i) {
            printf("for the car plate number %s, after just one character's change, %s is also OK.\n", res[0].data(), res[i].data());
        }
        */
    }

    //use 'a' for unsure character from 'A' to 'Z'
    //use 'd' for unsure character from '0' to '9'
    auto search_plates_fn = [&](const string& s, const vector<int>& offers) -> set<string> {
        function<void(int, string& t, vector<int>& left, set<string>& res)> helper_fn;
        helper_fn = [&](int i, string& t, vector<int>& left, set<string>& res) {
            if (i >= t.size()) {
                if (validate_fn(t)) res.insert(t);
                return;
            }
            char c = t[i];
            if ('a' == c) {
                for (char j = 'A'; j <= 'Z'; ++j) {
                    t[i] = j;
                    helper_fn(i + 1, t, left, res);
                    t[i] = c;
                }
            } else if ('d' == c) {
                for (auto& l : left) {
                    if (l < 0) continue;
                    if (10 == l) {
                        l = -l;
                        for (char j = '0'; j <= '9'; ++j) {
                            t[i] = j;
                            helper_fn(i + 1, t, left, res);
                            t[i] = c;
                        }
                        l = -l;
                    } else {
                        t[i] = '0' + l;
                        l = -l;
                        helper_fn(i + 1, t, left, res);
                        l = -l;
                        t[i] = c;
                    }
                }
            } else {
                helper_fn(i + 1, t, left, res);
            }
        };

        string t = s;
        auto left = offers;
        int d_cnt = 0;
        for (auto c : s) d_cnt += ('d' == c);
        while (d_cnt > left.size()) left.push_back(10);

        set<string> res;
        helper_fn(0, t, left, res);
        return res;
    };

    const auto& res = search_plates_fn("SMWddddS", {1, 6, 8});
    for (auto& s : res) {
        printf("%s\n", s.data());
    }
    return 0;
}
