#pragma once

#include <array>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "token.h"
#include "tokenpair.h"

typedef std::unordered_set<TokenPair,TokenPair::HashFunction> TokenPairSet;
typedef std::vector<TokenPair> TokenPairVector;

/*
 * Tokenizer class for tokenizing input text into tokens. 
 *
 * Normal operation steps:
 *   1. Feed input text to the tokenizer.
 *   2. Optimize token length until the maximum token count is reached or no more text is available
 *   3. Calculate token relationships.
 */
class Tokenizer 
{   
public:
    Tokenizer();
    // This class stores a lot of data, so we don't want to allow copying or moving it around.
    Tokenizer(const Tokenizer&) = delete;
    Tokenizer(Tokenizer&&) = delete;
    Tokenizer& operator=(const Tokenizer&) = delete;
    Tokenizer& operator=(Tokenizer&&) = delete;

    void FeedInput(const std::string& input); // add text to the tokenizer. can be called repeatedly
    void OptimizeTokenLength(size_t n = 1024); // combine smaller tokens until the list of tokens used reaches n
    void CalculateTokenRelationships(size_t depth = 5); // store all token relations found in input up to depth 

private:
    // all tokens, intermediate nodes and root nodes
    std::unordered_map<Token::data_type, Token> tokens; 
    Token::data_type nextToken = 0;

    // input data split in token sequences
    std::vector<std::vector<Token::data_type>> sequences;

    /*
     *   internal functions
     */

    // create all token pairs for one sequence
    TokenPairSet CreateTokenPairsForSequence(const std::vector<Token::data_type> & sequence) const;

    // create all tokenpairs for each sequence
    std::vector<TokenPairSet> CreateTokenPairsForSequences(const std::vector<std::vector<Token::data_type>> & sequences) const;

    // collect the tokenpairs found in sequences, and report the top n pairs found
    TokenPairVector GetTopTokenPairs(size_t n, const std::vector<TokenPairSet> & sequencesTokenPairs) const;

    // update sequences, sequencesTokenPairs and tokens by merging any pairs found in pairsForMerging 
    void MergeSpecificPairs(const TokenPairVector & pairsForMerging, std::vector<TokenPairSet> & sequencesTokenPairs);

};
