#pragma once

#include <array>
#include <set>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "db.h"
#include "token.h"
#include "tokenpair.h"

// TokenPairs stores the index of each sequence the TokenPair is used in
typedef std::unordered_map<TokenPair,std::multiset<size_t>,TokenPair::HashFunction> TokenPairs;
// Set of tokens
typedef std::unordered_set<Token,Token::HashFunction> TokensSet;
// Vector with sequences of Token::Data
typedef std::vector<Token::Data> TokenSequence;
typedef std::vector<TokenSequence> TokenSequences;
// vector with sequence ot TokenData
typedef std::vector<TokenPair> TokenPairSequence;
// vector with only the token and a count;
struct TokenCount
{
    Token::Data token;
    size_t count;
    bool operator==(const TokenCount & other) const
    {
        return token == other.token && count == other.count;
    }
};
typedef std::vector<TokenCount> TokenCounts;

/*
 * Tokenizer class for tokenizing input text into tokens. 
 *
 * Normal operation steps:
 *   1. Feed input text to the tokenizer
 *   2. Optimize token length until the maximum (used) token count is reached or no more text is available
 *   3. Calculate token relationships
 *   4. Store the results
 */
class Tokenizer 
{   
public:
    Tokenizer();
    Tokenizer(SQLite::DB & db);
    // This class stores a lot of data, so we don't want to allow copying or moving it around.
    Tokenizer(const Tokenizer&) = delete;
    Tokenizer(Tokenizer&&) = delete;
    Tokenizer& operator=(const Tokenizer&) = delete;
    Tokenizer& operator=(Tokenizer&&) = delete;

    // 1. add text to the tokenizer. can be called repeatedly
    void FeedInput(const std::string& input); 

    // 2. combine smaller tokens until the list of tokens used reaches n, in blocks of step.
    //    lower number of steps gives more accurate result, but takes longer
    void OptimizeTokenLength(size_t n = 1024, size_t step = 5);

    // 3. store all token relations found in input up to depth 
    void CalculateTokenRelationships(size_t depth = 5); 

    // 4. store token sequences and token relationships to a database
    void StoreCalculatedDataToDB(SQLite::DB & db) const;

private:
    // all tokens, intermediate nodes and root nodes
    TokensSet tokens; 
    size_t unusedTokensCount = 0;

    // input data split in token sequences
    TokenSequences sequences;

    // the relationship between any token and the n tokens before it
    // this map has less items than tokens, unused tokens are ignored
    // combining tokens and tokenRelations _migth_ work, but measure results
    std::unordered_map<Token::Data,std::vector<TokenCounts>> tokenRelations;

    /*
     *   internal functions
     */

    // return the number of used tokens
    size_t GetUsedTokensCount() const;

    // create tokenpairs for every sequence in sequences
    TokenPairs CreateTokenPairsForSequences();

    // collect the tokenpairs found in all sequences, and report the top n pairs found
    TokenPairSequence GetTopTokenPairs(size_t n, const TokenPairs & tokenPairs) const;

    // update sequences, tokenPairs, and tokens by merging any pairs found in pairsForMerging 
    void MergeTopTokenPairs(const TokenPairSequence & pairsForMerging, TokenPairs & tokenPairs);

    // get the original text sequence the token represents
    std::string GetTokenTextSequence(const Token & token) const;

    // load the data from db
    void LoadDataFromDB(SQLite::DB & db);

};
