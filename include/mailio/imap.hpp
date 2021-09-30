/*

imap.cpp
--------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#pragma once

#include <chrono>
#include <list>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <variant>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <cstdint>
#include "dialog.hpp"
#include "message.hpp"
#include "export.hpp"


namespace mailio
{


/**
IMAP client implementation.
**/
class MAILIO_EXPORT imap
{
public:

    /**
    Mailbox statistics structure.
    **/
    struct mailbox_stat_t
    {
        /**
        Statistics information to be retrieved.
        **/
        enum stat_info_t {DEFAULT = 0, UNSEEN = 1, UID_NEXT = 2, UID_VALIDITY = 4};

        /**
        Number of messages in the mailbox.
        **/
        unsigned long messages_no;

        /**
        Number of recent messages in the mailbox.
        **/
        unsigned long messages_recent;

        /**
        The non-zero number of unseen messages in the mailbox.

        Zero indicates the server did not report this and no assumptions can be made about the number of unseen messages.
        **/
        unsigned long messages_unseen;

        /**
        The non-zero message sequence number of the first unseen message in the mailbox.

        Zero indicates the server did not report this and no assumptions can be made about the first unseen message.
        **/
        unsigned long messages_first_unseen;

        /**
        The non-zero next unique identifier value of the mailbox.

        Zero indicates the server did not report this and no assumptions can be made about the next unique identifier.
        **/
        unsigned long uid_next;

        /**
        The non-zero unique identifier validity value of the mailbox.

        Zero indicates the server did not report this and does not support UIDs.
        **/
        unsigned long uid_validity;

        /**
        Setting the number of messages to zero.
        **/
        mailbox_stat_t() : messages_no(0), messages_recent(0), messages_unseen(0), messages_first_unseen(0), uid_next(0), uid_validity(0)
        {
        }
    };

    /**
    Mailbox folder tree.
    **/
    struct mailbox_folder_t
    {
        std::map<std::string, mailbox_folder_t> folders;
    };

    /**
    Available authentication methods.
    **/
    enum class auth_method_t {LOGIN};

    /**
    Single message ID or range of message IDs to be searched for.
    **/
    typedef std::pair<unsigned long, std::optional<unsigned long>> messages_range_t;

    /**
    Condition used by IMAP searching.

    It consists of key and value. Each key except the ALL has a value of the appropriate type: string, list of IDs, date.

    @todo Since both key and value types are known at compile time, perhaps they should be checked then instead at runtime.
    **/
    struct MAILIO_EXPORT search_condition_t
    {
        /**
        Condition key to be used as message search criteria.
        **/
        enum key_type {ALL, SID_LIST, UID_LIST, SUBJECT, FROM, TO, BEFORE_DATE, ON_DATE, SINCE_DATE} key;

        /**
        Condition value type to be used as message search criteria.

        Key ALL uses null because it does not need the value. Single ID can be given or range of IDs, or more than one range.
        **/
        typedef std::variant
        <
            std::monostate,
            std::string,
            std::list<messages_range_t>,
            boost::gregorian::date
        >
        value_type;

        /**
        Condition value itself.
        **/
        value_type value;

        /**
        String used to send over IMAP.
        **/
        std::string imap_string;

        /**
        Creating the IMAP string of the given condition.

        @param condition_key   Key to search for.
        @param condition_value Value to search for, default (empty) value is meant for the ALL key.
        @throw imap_error      Invaid search condition.
        **/
        search_condition_t(key_type condition_key, value_type condition_value = value_type());
    };

    /**
    Creating a connection to a server.

    @param hostname Hostname of the server.
    @param port     Port of the server.
    @param timeout  Network timeout after which I/O operations fail. If zero, then no timeout is set i.e. I/O operations are synchronous.
    @throw *        `dialog::dialog(const string&, unsigned)`.
    **/
    imap(const std::string& hostname, unsigned port, std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    /**
    Sending the logout command and closing the connection.
    **/
    virtual ~imap();

    imap(const imap&) = delete;

    imap(imap&&) = delete;

    void operator=(const imap&) = delete;

    void operator=(imap&&) = delete;

    /**
    Authenticating with the given credentials.

    The method should be called only once on an existing object - it is not possible to authenticate again within the same connection.

    @param username Username to authenticate.
    @param password Password to authenticate.
    @param method   Authentication method to use.
    @return         The server greeting message.

    @throw *        `connect()`, `auth_login(const string&, const string&)`.
    **/
    std::string authenticate(const std::string& username, const std::string& password, auth_method_t method);

    /**
    Selecting a mailbox.

    @param folder_name Folder to list.
    @return            Mailbox statistics.
    @throw imap_error  Selecting mailbox failure.
    @throw imap_error  Parsing failure.
    @throw *           `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo              Add server error messages to exceptions.
    @todo              Catch exceptions of `stoul` function.
    **/
    mailbox_stat_t select(const std::list<std::string>& folder_name, bool read_only = false);

    /**
    Selecting a mailbox.

    @param mailbox    Mailbox to select.
    @param read_only  Flag if the selected mailbox is only readable of also writable.
    @return           Mailbox statistics.
    @throw imap_error Selecting mailbox failure.
    @throw imap_error Parsing failure.
    @throw *          `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo             Add server error messages to exceptions.
    **/
    mailbox_stat_t select(const std::string& mailbox, bool read_only = false);

    /**
    Fetching a message from the mailbox.

    Some servers report success if a message with the given number does not exist, so the method returns with the empty `msg`. Other considers
    fetching non-existing message to be an error, and an exception is thrown.

    @param mailbox     Mailbox to fetch from.
    @param message_no  Number of the message to fetch.
    @param msg         Message to store the result.
    @param header_only Flag if only the message header should be fetched.
    @throw imap_error  Fetching message failure.
    @throw imap_error  Parsing failure.
    @throw *           `fetch(const list<messages_range_t>&, map<unsigned long, message>&, bool, bool, codec::line_len_policy_t)`.
    @todo              Add server error messages to exceptions.
    **/
    void fetch(const std::string& mailbox, unsigned long message_no, message& msg, bool header_only = false);

    /**
    Fetching a message from an already selected mailbox.

    A mailbox must already be selected before calling this method.

    Some servers report success if a message with the given number does not exist, so the method returns with the empty `msg`. Other considers
    fetching non-existing message to be an error, and an exception is thrown.

    @param message_no  Number of the message to fetch.
    @param msg         Message to store the result.
    @param is_uid      Using a message uid number instead of a message sequence number.
    @param header_only Flag if only the message header should be fetched.
    @throw *           `fetch(const list<messages_range_t>&, map<unsigned long, message>&, bool, bool, codec::line_len_policy_t)`.
    @todo              Add server error messages to exceptions.
    **/
    void fetch(unsigned long message_no, message& msg, bool is_uid = false, bool header_only = false);

    /**
    Fetching messages from an already selected mailbox.

    A mailbox must already be selected before calling this method.

    Some servers report success if a message with the given number does not exist, so the method returns with the empty `msg`. Other considers
    fetching non-existing message to be an error, and an exception is thrown.

    @param messages_range Range of message numbers or UIDs to fetch.
    @param found_messages Map of messages to store the results, indexed by message number or uid.
                          It does not clear the map first, so that results can be accumulated.
    @param is_uids        Using message UID numbers instead of a message sequence numbers.
    @param header_only    Flag if only the message headers should be fetched.
    @param line_policy    Decoder line policy to use while parsing each message.
    @throw imap_error     Fetching message failure.
    @throw imap_error     Parsing failure.
    @throw *              `parse_tag_result(const string&)`, `parse_response(const string&)`,
                          `dialog::send(const string&)`, `dialog::receive()`, `message::parse(const string&, bool)`.
    @todo                 Add server error messages to exceptions.
    **/
    void fetch(const std::list<messages_range_t>& messages_range, std::map<unsigned long, message>& found_messages, bool is_uids = false,
        bool header_only = false, codec::line_len_policy_t line_policy = codec::line_len_policy_t::RECOMMENDED);

    /**
    Appending a message to the given folder.

    @param folder_name Folder to append the message.
    @param msg         Message to append.
    @throw *           `append(const string&, const message&)`.
    **/
    void append(const std::list<std::string>& folder_name, const message& msg);

    /**
    Appending a message to the given folder.

    @param folder_name Folder to append the message.
    @param msg         Message to append.
    @throw imap_error  `Message appending failure.`, `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`,
                       `message::format(std::string&, bool)`.
    **/
    void append(const std::string& folder_name, const message& msg);

    /**
    Getting the mailbox statistics.

    The server might not support unseen, uidnext, or uidvalidity, which will cause an exception, so those parameters are optional.

    @param mailbox    Mailbox name.
    @param info       Statistics information to be retrieved.
    @return           Mailbox statistics.
    @throw imap_error Parsing failure.
    @throw imap_error Getting statistics failure.
    @throw *          `parse_tag_result(const string&)`, `parse_response(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo             Add server error messages to exceptions.
    @todo             Exceptions by `stoul()` should be rethrown as parsing failure.
    **/
    mailbox_stat_t statistics(const std::string& mailbox, unsigned int info = mailbox_stat_t::DEFAULT);


    /**
    Overload of the `statistics(const std::string&, unsigned int)`.

    @param folder_name Name of the folder to query for the statistics.
    @param info        Statistics information to be retrieved.
    @return            Mailbox statistics.
    @throw *           `statistics(const std::string&, unsigned int)`.
    **/
    mailbox_stat_t statistics(const std::list<std::string>& folder_name, unsigned int info = mailbox_stat_t::DEFAULT);

    /**
    Removing a message from the given mailbox.

    @param mailbox    Mailbox to use.
    @param message_no Number of the message to remove.
    @param is_uid     Using a message uid number instead of a message sequence number.
    @throw imap_error Deleting message failure.
    @throw imap_error Parsing failure.
    @throw *          `select(const string&)`, `parse_tag_result(const string&)`, `remove(unsigned long, bool)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo             Add server error messages to exceptions.
    **/
    void remove(const std::string& mailbox, unsigned long message_no, bool is_uid = false);

    /**
    Removing a message from the given mailbox.

    @param mailbox    Mailbox to use.
    @param message_no Number of the message to remove.
    @param is_uid     Using a message uid number instead of a message sequence number.
    @throw *          `remove(const string&, bool)`.
    @todo             Add server error messages to exceptions.
    **/
    void remove(const std::list<std::string>& mailbox, unsigned long message_no, bool is_uid = false);

    /**
    Removing a message from an already selected mailbox.

    @param message_no Number of the message to remove.
    @param is_uid     Using a message uid number instead of a message sequence number.
    @throw imap_error Deleting message failure.
    @throw imap_error Parsing failure.
    @throw *          `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo             Add server error messages to exceptions.
    @todo             Catch exceptions of `stoul` function.
    **/
    void remove(unsigned long message_no, bool is_uid = false);

    /**
    Searching a mailbox.

    @param conditions  List of conditions taken in conjuction way.
    @param results     Store resulting list of message sequence numbers or UIDs here.
                       Does not clear the list first, so that results can be accumulated.
    @param want_uids   Return a list of message UIDs instead of message sequence numbers.
    @throw imap_error  Search mailbox failure.
    @throw imap_error  Parsing failure.
    @throw *           `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo              Add server error messages to exceptions.
    **/
    void search(const std::list<search_condition_t>& conditions, std::list<unsigned long>& results, bool want_uids = false);

    /**
    Creating folder.

    @param folder_name Folder to be created.
    @return            True if created, false if not.
    @throw imap_error  Parsing failure.
    @throw imap_error  Creating folder failure.
    @throw *           `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo              Return status really needed?
    **/
    bool create_folder(const std::string& folder_name);

    /**
    Creating folder.

    @param folder_name Folder to be created.
    @return            True if created, false if not.
    @throw *           `folder_delimiter()`, `create_folder(const string&)`.
    @todo              Return status really needed?
    **/
    bool create_folder(const std::list<std::string>& folder_name);

    /**
    Listing folders.

    @param folder_name Folder to list.
    @return            Subfolder tree of the folder.
    @throw imap_error  Listing folders failure.
    @throw imap_error  Parsing failure.
    @throw *           `folder_delimiter()`, `parse_tag_result`, `dialog::send(const string&)`, `dialog::receive()`.
    **/
    mailbox_folder_t list_folders(const std::string& folder_name);

    /**
    Listing folders.

    @param folder_name Folder to list.
    @return            Subfolder tree of the folder.
    @throw *           `folder_delimiter()`, `list_folders(const string&)`.
    **/
    mailbox_folder_t list_folders(const std::list<std::string>& folder_name);

    /**
    Deleting a folder.

    @param folder_name Folder to delete.
    @return            True if deleted, false if not.
    @throw imap_error  Parsing failure.
    @throw imap_error  Deleting folder failure.
    @throw *           `folder_delimiter()`, `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo              Return status really needed?
    **/
    bool delete_folder(const std::string& folder_name);

    /**
    Deleting a folder.

    @param folder_name Folder to delete.
    @return            True if deleted, false if not.
    @throw *           `delete_folder(const string&)`.
    @todo              Return status really needed?
    **/
    bool delete_folder(const std::list<std::string>& folder_name);

    /**
    Renaming a folder.

    @param old_name    Old name of the folder.
    @param new_name    New name of the folder.
    @return            True if renaming is successful, false if not.
    @throw imap_error  Parsing failure.
    @throw imap_error  Renaming folder failure.
    @throw *           `folder_delimiter()`, `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo              Return status really needed?
    **/
    bool rename_folder(const std::string& old_name, const std::string& new_name);

    /**
    Renaming a folder.

    @param old_name    Old name of the folder.
    @param new_name    New name of the folder.
    @return            True if renaming is successful, false if not.
    @throw *           `rename_folder(const string&, const string&)`.
    @todo              Return status really needed?
    **/
    bool rename_folder(const std::list<std::string>& old_name, const std::list<std::string>& new_name);

    /**
    Determining folder delimiter of a mailbox.

    It is required to know the folder delimiter string in case one wants to deal with the folder names as strings.

    @return           Folder delimiter.
    @throw imap_error Determining folder delimiter failure.
    @throw *          `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    **/
    std::string folder_delimiter();

protected:

    /**
    Formatting range of IDs to a string.

    @param id_pair Range of IDs to format.
    @return        Range od IDs as IMAP grammar string.
    **/
    static std::string messages_range_to_string(messages_range_t id_pair);

    /**
    Formatting list of ranges of IDs to a string.

    @param ranges List of ID ranges to format.
    @return       List of ranges of IDs as IMAP grammar string.
    **/
    static std::string messages_range_list_to_string(std::list<messages_range_t> ranges);

    /**
    Untagged response character as defined by the protocol.
    **/
    static const std::string UNTAGGED_RESPONSE;

    /**
    Continuation response character as defined by the protocol.
    **/
    static const std::string CONTINUE_RESPONSE;

    /**
    Colon as a separator in the message list range.
    **/
    static const std::string RANGE_SEPARATOR;

    /**
    Character to mark all messages until the end of range.
    **/
    static const std::string RANGE_ALL;

    /**
    Comma as a separator of the list members.
    **/
    static const std::string LIST_SEPARATOR;

    /**
    Character used by IMAP to separate tokens.
    **/
    static const char TOKEN_SEPARATOR_CHAR{' '};

    /**
    String representation of the token separator character.
    **/
    static const std::string TOKEN_SEPARATOR_STR;

    /**
    Quoted string delimiter.
    **/
    static const char QUOTED_STRING_SEPARATOR_CHAR{'"'};

    /**
    String representation of the quoted string delimiter character.
    **/
    static const std::string QUOTED_STRING_SEPARATOR;

    /**
    Character which begins the optional section.
    **/
    static const char OPTIONAL_BEGIN{'['};

    /**
    Character which ends the optional section.
    **/
    static const char OPTIONAL_END{']'};

    /**
    Character which begins the list.
    **/
    static const char LIST_BEGIN{'('};

    /**
    Character which ends the list.
    **/
    static const char LIST_END{')'};

    /**
    Character which begins the literal string.
    **/
    static const char STRING_LITERAL_BEGIN{'{'};

    /**
    Character which ends the literal string.
    **/
    static const char STRING_LITERAL_END{'}'};

    /**
    Delimiter of a quoted atom in the protocol.
    **/
    static const char QUOTED_ATOM{'"'};

    /**
    Initiating a session to the server.

    @return           The server greeting message.
    @throw imap_error Connection to server failure.
    @throw imap_error Parsing failure.
    @throw *          `parse_tag_result(const string&)`, `dialog::receive()`.
    @todo             Add server error messages to exceptions.
    **/
    std::string connect();

    /**
    Performing an authentication by using the login method.

    @param username   Username to authenticate.
    @param password   Password to authenticate.
    @throw imap_error Authentication failure.
    @throw imap_error Parsing failure.
    @throw *          `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo             Add server error messages to exceptions.
    **/
    void auth_login(const std::string& username, const std::string& password);

    /**
    Searching a mailbox.

    @param conditions  String of search keys.
    @param results     Store resulting list of indexes here.
    @param want_uids   Return a list of message uids instead of message sequence numbers.
    @throw imap_error  Search mailbox failure.
    @throw imap_error  Parsing failure.
    @throw *           `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo              Add server error messages to exceptions.
    **/
    void search(const std::string& conditions, std::list<unsigned long>& results, bool want_uids = false);

    /**
    Parsed elements of IMAP response line.
    **/
    struct tag_result_response_t
    {
        /**
        Possible response results.
        **/
        enum result_t {OK, NO, BAD};

        /**
        Tag of the response.
        **/
        std::string tag;

        /**
        Result of the response, if exists.
        **/
        std::optional<result_t> result;

        /**
        Rest of the response line.
        **/
        std::string response;

        tag_result_response_t() = default;

        /**
        Initializing the tag, result and rest of the line with the given values.
        **/
        tag_result_response_t(const std::string& parsed_tag, const std::optional<result_t>& parsed_result, const std::string& parsed_response) :
            tag(parsed_tag), result(parsed_result), response(parsed_response)
        {
        }

        tag_result_response_t(const tag_result_response_t&) = delete;

        tag_result_response_t(tag_result_response_t&&) = delete;

        ~tag_result_response_t() = default;

        tag_result_response_t& operator=(const tag_result_response_t&) = delete;

        tag_result_response_t& operator=(tag_result_response_t&&) = delete;

        /**
        Formatting the response line to a user friendly format.

        @return Response line as string.
        **/
        std::string to_string() const;
    };

    /**
    Parsing a line into tag, result and response which is the rest of the line.

    @param line       Response line to parse.
    @return           Tuple with the tag, result and response.
    @throw imap_error Parsing failure.
    */
    tag_result_response_t parse_tag_result(const std::string& line) const;

    /**
    Parsing a response (without tag and result) into optional and mandatory part.

    This is the main function that deals with the IMAP grammar.

    @param response   Response to parse without tag and result.
    @throw imap_error Parser failure.
    @throw *          `std::stoul`.
    @todo             Perhaps the error should point to a part of the string where the parsing fails.
    **/
    void parse_response(const std::string& response);

    /**
    Resetting the parser state to the initial one.
    **/
    void reset_response_parser();

    /**
    Formatting a tagged command.

    @param command Command to format.
    @return        New tag as string.
    **/
    std::string format(const std::string& command);

    /**
    Trimming trailing CR character.

    @param line Line to trim.
    **/
    void trim_eol(std::string& line);

    /**
    Formatting folder tree to string.

    @param folder_tree Folders to format into string.
    @param delimiter   Delimiter of the folders.
    @return            Formatted string.
    **/
    std::string folder_tree_to_string(const std::list<std::string>& folder_tree, std::string delimiter) const;

    /**
    Converting gregorian date to string required by IMAP searching condition.

    @param gregorian_date Gregorian date to convert.
    @return               Date as string required by IMAP search condition.
    @todo                 Static method of `search_condition_t` structure?
    **/
    static std::string imap_date_to_string(const boost::gregorian::date& gregorian_date);

    /**
    Dialog to use for send/receive operations.
    **/
    std::shared_ptr<dialog> _dlg;

    /**
    Tag used to identify requests and responses.
    **/
    unsigned _tag;

    /**
    Token of the response defined by the grammar.

    Its type is determined by the content, and can be either atom, string literal or parenthesized list. Thus, it can be considered as union of
    those three types.
    **/
    struct response_token_t
    {
        /**
        Token type which can be empty in the case that is not determined yet, atom, string literal or parenthesized list.
        **/
        enum class token_type_t {EMPTY, ATOM, LITERAL, LIST} token_type;

        /**
        Token content in case it is atom.
        **/
        std::string atom;

        /**
        Token content in case it is string literal.
        **/
        std::string literal;

        /**
        String literal is first determined by its size, so it's stored here before reading the literal itself.
        **/
        std::string literal_size;

        /**
        Token content in case it is parenthesized list.

        It can store either of the three types, so the definition is recursive.
        **/
        std::list<std::shared_ptr<response_token_t>> parenthesized_list;

        /**
        Default constructor.
        **/
        response_token_t() : token_type(token_type_t::EMPTY)
        {
        }
    };

    /**
    Optional part of the response, determined by the square brackets.
    **/
    std::list<std::shared_ptr<response_token_t>> _optional_part;

    /**
    Mandatory part of the response, which is any text outside of the square brackets.
    **/
    std::list<std::shared_ptr<response_token_t>> _mandatory_part;

    /**
    Parser state if an optional part is reached.
    **/
    bool _optional_part_state;

    /**
    Parser state if an atom is reached.
    **/
    enum class atom_state_t {NONE, PLAIN, QUOTED} _atom_state;

    /**
    Counting open parenthesis of a parenthized list, thus it also keeps parser state if a parenthesized list is reached.
    **/
    unsigned int _parenthesis_list_counter;

    /**
    Parser state if a string literal is reached.
    **/
    enum class string_literal_state_t {NONE, SIZE, WAITING, READING, DONE} _literal_state;

    /**
    Keeping the number of bytes read so far while parsing a string literal.
    **/
    std::string::size_type _literal_bytes_read;

    /**
    Finding last token of the list at the given depth in terms of parenthesis count.

    When a new token is found, this method enables to find the last current token and append the new one.

    @param token_list Token sequence to traverse.
    @return           Last token of the given sequence at the current depth of parenthesis count.
    **/
    std::list<std::shared_ptr<response_token_t>>* find_last_token_list(std::list<std::shared_ptr<response_token_t>>& token_list);

    /**
    Keeping the number of end-of-line characters to be counted as additionals to a formatted line.

    If CR is removed, then two characters are counted as additional when a literal is read. If CR is not removed, then only LF was removed
    during network read, so one character is counted as additional when a literal is read.

    This is necessary for cases when the protocol returns literal with lines ended with LF only. Not sure is that is the specification violation,
    perhaps CRLF at the end of each line read from network is necessary.

    @todo Check if this is breaking protocol, so it has to be added to a strict mode.
    **/
    std::string::size_type _eols_no;
};


/**
Secure version of `imap` class.
**/
class MAILIO_EXPORT imaps : public imap
{
public:

    /**
    Available authentication methods.
    **/
    enum class auth_method_t {LOGIN, START_TLS};

    /**
    Making a connection to the server.

    Calls parent constructor to do all the work.

    @param hostname Hostname of the server.
    @param port     Port of the server.
    @param timeout  Network timeout after which I/O operations fail. If zero, then no timeout is set i.e. I/O operations are synchronous.
    @throw *        `imap::imap(const std::string&, unsigned)`.
    **/
    imaps(const std::string& hostname, unsigned port, std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    /**
    Sending the logout command and closing the connection.

    Calls parent destructor to do all the work.
    **/
    virtual ~imaps() = default;

    imaps(const imap&) = delete;

    imaps(imaps&&) = delete;

    void operator=(const imaps&) = delete;

    void operator=(imaps&&) = delete;

    /**
    Authenticating with the given credentials.

    @param username Username to authenticate.
    @param password Password to authenticate.
    @param method   Authentication method to use.
    @throw *        `connect()`, `switch_to_ssl()`, `start_tls()`, `auth_login(const std::string&, const std::string&)`.
    **/
    std::string authenticate(const std::string& username, const std::string& password, auth_method_t method);

protected:

    /**
    Switching to TLS layer.

    @throw imap_error Bad server response.
    @throw imap_error Start TLS refused by server.
    @throw *          `parse_tag_result(const std::string&)`, `switch_to_ssl()`, `dialog::send(const std::string&)`, `dialog::receive()`.
    **/
    void start_tls();

    /**
    Replacing TCP socket with SSL socket.

    @throw * `dialog_ssl::dialog_ssl(dialog_ssl&&)`.
    **/
    void switch_to_ssl();
};


/**
Error thrown by IMAP client.
**/
class imap_error : public std::runtime_error
{
public:

    /**
    Calling parent constructor.

    @param msg  Error message.
    **/
    explicit imap_error(const std::string& msg) : std::runtime_error(msg)
    {
    }

    /**
    Calling parent constructor.

    @param msg  Error message.
    **/
    explicit imap_error(const char* msg) : std::runtime_error(msg)
    {
    }
};


} // namespace mailio
