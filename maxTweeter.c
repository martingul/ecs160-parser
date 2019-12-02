/*
    input: csv file
    - convert file to array of lines
        ["h0,h1,h2,...", "a,b,c,...", ...]
    - convert to splitted array of lines
        []["h0", "h1", "h2", "..."], ["a", "b", "c", "..."], ...]
    - convert to array of maps
    - sort and print 10 elements
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// metadata about file
struct metadata {
    size_t num_tweets;
    size_t num_cols;
    size_t name_offset;
};

// key-value map, where key is a string and value an integer
struct map {
    char* key;
    size_t value;
};

const size_t LINE_SIZE = 1024;

void error(char*);
int comp_map(const void*, const void*);
size_t get_offset(char**, char*, size_t);
char** create_tweets_from_csv(char*, struct metadata*);
char** split_line(char*, struct metadata*);

// output an error and exit
void error(char* message) {
    printf("Error: %s\n", message);
    exit(1);
}

// compare two maps by value
int comp_map(const void* lhs, const void* rhs) {
    struct map* lhs_map = (struct map*) lhs;
    struct map* rhs_map = (struct map*) rhs;

    if (lhs_map->value > rhs_map->value) return 1;
    if (lhs_map->value < rhs_map->value) return -1;
    return 0;
}

// get row integer offset for given column name
// e.g. id,name,text -> name returns 1
size_t get_offset(char** cols, char *col_name, size_t num_cols) {
    size_t offset = 0;

    // iterate through each columns to find matching name
    for (size_t i = 0; i < num_cols; i++) {
        if (strncmp(col_name, cols[i], strlen(col_name)) == 0)
            return offset; // return matching offset
        else
            offset++;
    }

    return offset;
}

// read a csv file from disk and return a list of tweets
char** create_tweets_from_csv(char* filename, struct metadata* m) {
    FILE* fp = fopen(filename, "r");
    if (!fp) error("cannot open file");

    // read header line
    char header_line[LINE_SIZE];
    fgets(header_line, LINE_SIZE, fp);
    char** header_split = split_line(header_line, m);

    // get name column get_offset
    m->name_offset = get_offset(header_split, "name", m->num_cols);

    // free header line
    for (size_t i = 0; i < LINE_SIZE; i++) free(header_split[i]);
    free(header_split);

    // check if file has more than two lines
    if (feof(fp)) error("tweet list empty\n");

    // read rest of the file
    size_t num_lines = 1;
    char line[LINE_SIZE]; // current line being read
    char** lines = malloc(num_lines * LINE_SIZE); // array of lines, each line being a tweet
    if (lines == NULL) error("memory allocation");

    // create an array of lines from file
    while (fgets(line, LINE_SIZE, fp)) {
        lines = realloc(lines, num_lines * LINE_SIZE); // resize array of lines
        if (lines == NULL) error("memory allocation");

        char* new_line = malloc(LINE_SIZE); // allocate line
        if (new_line == NULL) error("memory allocation");

        lines[num_lines - 1] = new_line;
        memcpy(lines[num_lines - 1], line, LINE_SIZE); // copy content of line into array of lines
        num_lines++;
        // TODO test if num_cols match
    }

    m->num_tweets = --num_lines; // set number of total tweets

    return lines;
}

// split line delimited by commas into array of strings
// TODO validate opening/closing quotes
char** split_line(char* line, struct metadata* m) {
    // array of splitted columns to return
    char** splitted_cols = malloc(LINE_SIZE * sizeof(char*));
    if (splitted_cols == NULL) error("memory allocation");

    // allocate array of strings
    for (size_t i = 0; i < LINE_SIZE; i++) {
        char* splitted_col = malloc(LINE_SIZE);
        if (splitted_col == NULL) error("memory allocation");
        splitted_cols[i] = splitted_col;
    }

    size_t col_i = 0, split_i = 0; // buffer indexes
    char* col = malloc(LINE_SIZE); // current column buffer
    if (col == NULL) error("memory allocation");

    // iterate through each character of line
    for (char* c = line; *c != '\0'; c++) {
        // load characters into col until we reach a comma
        // then append col to array of splitted cols
        if (*c == ',') {
            // copy current column to splitted columns array
            memcpy(splitted_cols[split_i], col, LINE_SIZE);
            split_i++;

			// TODO validate for quotes

            // reset the current buffer
            memset(col, 0, LINE_SIZE);
            col_i = 0;
        } else {
            // append to current column buffer
            col[col_i] = *c;
            col_i++;
        }
    }

    // copy only / last column and trim new line
    char col_trim[col_i];
    strncpy(col_trim, col, col_i - 1);
    memcpy(splitted_cols[split_i], col_trim, col_i);

    m->num_cols = split_i + 1; // set the number of columns
    free(col); // free column buffer

    return splitted_cols;
}

int main(int argc, char** argv) {
    // TODO
    // - no additional commas inside the tweet text
    // - two columns with same name should result in error
    if (argc != 2) {
        printf("Usage: %s <file.csv>\n", argv[0]);
        return 1;
    };

    // csv file metadata for processing
    struct metadata m = { 0, 0, 0 };

    // read tweets from csv file
    char** tweets = create_tweets_from_csv(argv[1], &m);

    size_t i = 0, num_tweeters = 0; // number of unique tweeters
    struct map tweeters[m.num_tweets]; // map { tweeter => tweet_count }

    // from each tweet, create a map with key being the tweeter name
    // and value the total tweet count, append it to an array of maps
    // holding total tweet count of every tweeter
    for (i = 0; i < m.num_tweets; i++) {
        char** tweet = split_line(tweets[i], &m); // split line into array
        char* tweeter = tweet[m.name_offset]; // extract "name" column value

        // skip if tweeter is an empty string
        if (strlen(tweeter) > 0) {
            int is_counted = 0;
            struct map* tweeter_map = NULL;

            // check if map is already in array
            for (size_t j = 0; j < num_tweeters; j++) {
                if (strncmp(tweeter, tweeters[j].key, strlen(tweeter)) == 0) {
                    tweeter_map = &tweeters[j]; // keep reference to matching map
                    is_counted = 1;
                }
            }

            if (is_counted == 1 && tweeter_map != NULL) {
                // if map is already in array, increase count of reference map
                tweeter_map->value++;
                is_counted = 0;
            } else {
                // otherwise insert a new map with initial count of one
                struct map tweeter_map = { tweeter, 1 };
                tweeters[num_tweeters] = tweeter_map;
                num_tweeters++;
            }
        }
    }

    // sort the array of maps by value of tweet count
    qsort(tweeters, num_tweeters, sizeof(struct map), comp_map);

    // output the 10 last elements which have greatest counts
    for (i = 1; i <= num_tweeters && i <= 10; i++) {
        printf("%s: %ld\n",
            tweeters[num_tweeters - i].key,
            tweeters[num_tweeters - i].value);
    }

    // free allocated memory
    for (i = 0; i < m.num_tweets; i++) free(tweets[i]);
    free(tweets);
    for (i = 0; i < num_tweeters; i++) free(tweeters[i].key);
}
