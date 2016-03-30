#pragma once

#include "path_builder.hpp"
#include "simple_config_origin.hpp"
#include "tokenizer.hpp"
#include <hocon/config_syntax.hpp>
#include <internal/nodes/config_node_path.hpp>

#include <string>
#include <vector>

namespace hocon  {

    class path_parser {
    public:
        static config_node_path parse_path_node(std::string const& path_string,
                                                config_syntax flavor = config_syntax::CONF);

        static path parse_path(std::string const& path_string);

        template <typename iter>
        static path parse_path_expression(iter begin, iter end, shared_origin origin,
                                          std::string const& original_text = "",
                                          token_list* path_tokens = nullptr,
                                          config_syntax flavor = config_syntax::CONF)
        {
            using namespace std;

            vector<element> elements = { element("", false) };

            if (begin == end) {
                throw config_exception("Invalid path: " + original_text +
                                       " -- Expecting a field name or path here, but got nothing");
            }

            do {
                shared_token t = *begin;

                if (path_tokens != nullptr) {
                    path_tokens->push_back(t);
                }

                if (t->get_token_type() == token_type::IGNORED_WHITESPACE) {
                    continue;
                }

                if (tokens::is_value_with_type(t, config_value::type::STRING)) {
                    auto value = tokens::get_value(t);
                    // this is a quoted string, so any periods in here don't count as path separators
                    string s = value->render();
                    add_path_text(elements, true, s);
                } else if (t == tokens::end_token()) {
                    // ignore this; when parsing a file, it should not happen
                    // since we're parsing a token list rather than the main
                    // token iterator, and when parsing a path expression from the
                    // API, it's expected to have an END.
                } else {
                    // any periods outside of a quoted string count as separators
                    string text;
                    if (t->get_token_type() == token_type::VALUE) {
                        // appending a number here may add a period,
                        // but we _do_ count those as path separators,
                        // because we basically want "foo 3.0bar" to
                        // parse as a string even though there's a
                        // number in it. The fact that we tokenize
                        // non-string values is largely an
                        // implementation detail.
                        auto value = tokens::get_value(t);

                        // We need to split the tokens on a . so that
                        // we can get sub-paths but still preserve the
                        // original path text when doing an insertion
                        if (path_tokens != nullptr) {
                            path_tokens->pop_back();
                            token_list split_tokens = split_token_on_period(t, flavor);
                            path_tokens->insert(path_tokens->end(), split_tokens.begin(), split_tokens.end());
                        }
                        text = value->render();
                    } else if (t->get_token_type() == token_type::UNQUOTED_TEXT) {
                        // We need to split the tokens on a . so that
                        // we can get sub-paths but still preserve the
                        // original path text when doing an insertion
                        // on ConfigNodeObjects
                    if (path_tokens != nullptr) {
                        path_tokens->pop_back();
                        token_list split_tokens = split_token_on_period(t, flavor);
                        path_tokens->insert(path_tokens->end(), split_tokens.begin(), split_tokens.end());
                    }
                    text = t->token_text();
                    } else {
                        throw config_exception("Invalid path: " + original_text +
                                               " -- Token not allowed in path expression: " +
                                               t->to_string() +
                                               " (you can double-quote this token if you really want it here)");
                    }

                    add_path_text(elements, false, text);
                }
            } while(++begin != end);

            path_builder builder;
            for (element e : elements) {
                if (e._value.length() == 0 && !e._can_be_empty) {
                    throw config_exception("Invalid path: " + original_text +
                                           " -- path has a leading, trailing, or two adjacent '.'" +
                                           " (use quoted \"\" empty string if you want an empty element)");
                } else {
                    builder.append_key(e._value);
                }
            }

            return builder.result();
        }


        static config_node_path parse_path_node_expression(iterator& expression,
                                                           shared_origin origin,
                                                           std::string const& original_text = "",
                                                           config_syntax flavor = config_syntax::CONF);

    private:
        class element {
        public:
            element(std::string initial, bool can_be_empty);

            std::string to_string() const;

            std::string _value;
            bool _can_be_empty;
        };

        static token_list split_token_on_period(shared_token t, config_syntax flavor);

        static void add_path_text(std::vector<element>& buff, bool was_quoted, std::string new_text);

        static bool looks_unsafe_for_fast_parser(std::string s);

        static path fast_path_build(path tail, std::string s);

        static path speculative_fast_parse_path(std::string const& path);

        static const shared_origin api_origin;
    };

}  // namespace hocon
