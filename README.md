# Web-Server-Crawler-Implementation
Web crawler implementation with the aim of downloading all web sites from the web server and at the same time serving search commands


  # webcreator.sh
      (./webcreator.sh root_directory text_file w p)
    webcreator.sh application work as follows:
    first is checking that the directory and
    the file given as input also exists and that the parameters w and p are integers. It is also checking
    that the text_file has at least 10,000 lines. Then it is creating in the root_directory w directories that each represents a web site.
    These directories are in the form of site0, site1, ..., sitew-1. Then it is creating p
    pages in each directory that is representing the websites of each web site. Websites are
    having the name pagei_j.html where i is the web site and j is a random, unique
    each web site, number.
    For example, if w = 3 and p = 2 the following directories and pages will be created:

    root_dir/site0/
    root_dir/site0/page0_2345.html
    root_dir/site0/page0_2391.html
    root_dir/site1/
    root_dir/site1/page1_2345.html
    root_dir/site1/page1_2401.html
    root_dir/site2/
    root_dir/site2/page2_6530.html
    root_dir/site2/page2_16672.html



  # Web server (myhttpd.c)
      (./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir)
    myhttpd.c work as follows:
    Simply accepts HTTP/1.1 requests. For each the server assigns it to one of the threads from the thread pool it has. THE
    thread work is to return the root_dir/ site0/page0_1244.html file back to him who asked for it.

      The operation of the server with the threads is as follows:
    the server accepts a connection to the socket and places the corresponding file descriptor in a buffer from
    which the threads read. Each thread is responsible for a file descriptor from which it will
    read the GET request and it will return the answer.

      Finally, in the command port, the server listens and accepts the following simple commands (1 word each) which
    run directly by the server without having to be assigned to a thread:
      STATS: the server responds with how many hours it runs, how many pages it has returned and the total number of bytes.
        e.g.:
        Server up for 02: 03.45, served 124 pages, 345539 bytes
      SHUTDOWN: the server stops serving additional requests



  # Web crawler (mycrawler.c)
    (./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL)
      The job of the crawler (in C / C ++) is to download all web sites from the web server by downloading
    pages, analyzing them and finding links in the pages that should follow retrospectively.
    The crawler works as follows:
      1. Starting, it creates a thread pool with the corresponding threads. The threads are reused.
      It also creates a queue in which to store the links that have been found so far and puts it
      starting_URL in this queue.
      2. One of the threads takes the URL from the queue and requests it from the server. The URL is hers
      format.
      3. After downloading the file, it saves it to the corresponding directory / file in save_dir.
      4. Analyzes the file that has just been downloaded and finds other links which it puts in the queue.
      5. Repeats the process from 2 with all the threads in parallel until there are no other links
      in queue.

      At the command port the crawler listens and accepts the following simple commands (1 word each) which
      performed directly by the crawler without having to be assigned to a thread:

        STATS: the crawler answers how long it takes, how many pages it has collected and the total number
          bytes. E.g.:
          Crawler up for 01: 06.45, downloaded 124 pages, 345539 bytes
        SEARCH word1 word2 word3 ... word10
          If the crawler still has pages in the queue then it returns a message indicating that crawling
          is in-progress.
        SHUTDOWN: the crawler stops asking for additional pages and stops its execution.
