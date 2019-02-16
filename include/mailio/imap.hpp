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
#include <stdexcept>
#include <string>
#include <tuple>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/streambuf.hpp>
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
        Number of messages in the mailbox.
        **/
        unsigned long messages_no;

        /**
        Setting the number of messages to zero.
        **/
        mailbox_stat_t() : messages_no(0)
        {
        }
    };

    /**
    Mailbox folder tree.
    **/
    struct mailbox_folder
    {
        std::map<std::string, mailbox_folder> folders;
    };

    /**
    Available authentication methods.
    **/
    enum class auth_method_t {LOGIN};

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
    @throw *        `connect()`, `auth_login(const string&, const string&)`.
    **/
    void authenticate(const std::string& username, const std::string& password, auth_method_t method);

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
    @throw *           `parse_tag_result(const string&)`, `select(const string&)`, `parse_response(const string&)`,
                       `dialog::send(const string&)`, `dialog::receive()`, `message::parse(const string&, bool)`.
    @todo              Add server error messages to exceptions.
    **/
    void fetch(const std::string& mailbox, unsigned long message_no, message& msg, bool header_only = false);

    /**
    Getting the mailbox statistics.
    
    @param mailbox    Mailbox name.
    @return           Mailbox statistics.
    @throw imap_error Parsing failure.
    @throw imap_error Getting statistics failure.
    @throw *          `parse_tag_result(const string&)`, `parse_response(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo             Add server error messages to exceptions.
    **/
    mailbox_stat_t statistics(const std::string& mailbox);

    /**
    Removing a message from the given mailbox.
    
    @param mailbox    Mailbox to use.
    @param message_no Number of the message to remove.
    @throw imap_error Deleting message failure.
    @throw imap_error Parsing failure.
    @throw *          `select(const string&)`, `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo             Add server error messages to exceptions.
    **/
    void remove(const std::string& mailbox, unsigned long message_no);

    /**
    Creating folder.

    @param folder_tree Folder to be created.
    @return            True if created, false if not.
    @throw imap_error  Parsing failure.
    @throw imap_error  Creating folder failure.
    @throw *           `folder_delimiter()`, `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    **/
    bool create_folder(const std::list<std::string>& folder_tree);

    /**
    Listing folders.

    @param folder_name Folder to list.
    @return            Subfolder tree of the folder.
    **/
    mailbox_folder list_folders(const std::list<std::string>& folder_name);

    /**
    Deleting a folder.

    @param folder_name Folder to delete.
    @return            True if deleted, false if not.
    @throw imap_error  Parsing failure.
    @throw imap_error  Deleting folder failure.
    @throw *           `folder_delimiter()`, `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
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
    **/
    bool rename_folder(const std::list<std::string>& old_name, const std::list<std::string>& new_name);

protected:

    /**
    Initiating a session to the server.
    
    @throw imap_error Connection to server failure.
    @throw imap_error Parsing failure.
    @throw *          `parse_tag_result(const string&)`, `dialog::receive()`.
    @todo             Add server error messages to exceptions.
    **/
    void connect();

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
    Selecting a mailbox.
    
    @param mailbox    Mailbox to select.
    @throw imap_error Selecting mailbox failure.
    @throw imap_error Parsing failure.
    @throw *          `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo             Add server error messages to exceptions.
    **/
    void select(const std::string& mailbox);

    /**
    Determining folder delimiter of a mailbox.

    @return           Folder delimiter.
    @throw imap_error Determining folder delimiter failure.
    @throw *          `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    **/
    std::string folder_delimiter();

    /**
    Parsing a line into tag, result and response which is the rest of the line.
    
    @param line       Response line to parse.
    @return           Tuple with the tag, result and response.
    @throw imap_error Parsing failure.
    */
    std::tuple<std::string, std::string, std::string> parse_tag_result(const std::string& line) const;
    
    /**
    Parsing a response (without tag and result) into optional and mandatory part.
    
    This is the main function that deals with the IMAP grammar.

    @param response   Response to parse without tag and result.
    @throw imap_error Parser failure.
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
    Dialog to use for send/receive operations.
    **/
    std::unique_ptr<dialog> _dlg;

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
    void authenticate(const std::string& username, const std::string& password, auth_method_t method);

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
