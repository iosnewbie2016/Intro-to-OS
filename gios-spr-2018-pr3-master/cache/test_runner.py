from subprocess import Popen, PIPE, STDOUT, call
from time import sleep, time
from signal import SIGINT
import os

root = 'test_results/'

def multi_cache_same_file():
    print("Multiple Cache Threads Single Server")
    test = 'multi-cache-same-file-'
    file_loc = root + test

    webproxy_out = open(file_loc + 'webproxy.out', 'w')
    simplecached_out = open(file_loc + 'simplecached.out', 'w')
    gfclient_out = open(file_loc + 'gfclient.out', 'w')

    simplecached = Popen(['./simplecached', '-t 8'], shell=False, stdout=simplecached_out, stderr=simplecached_out)
    webproxy = Popen('./webproxy', shell=False, stdout=webproxy_out, stderr=webproxy_out)
    sleep(1)
    start = time()
    client = Popen('./gfclient_measure -p 6200 -t 16', shell=True, stdout=gfclient_out, stderr=gfclient_out)
    while client.poll() == None:
        if time() - start > 10:  # Give up after 10 seconds
            break
    end = time()
    print("Elapsed time {}".format(end - start))
    if client.poll() == None:
        print('Server taking too long to respond. Client probably hung.')
        client.send_signal(SIGINT)
    else:
        print('Client finished in {}. exit code: {}'.format(end-start, client.poll()))
    webproxy.terminate()
    simplecached.terminate()

# Multiple Cache Threads, Single Server Thread
def multi_cache_single_server():
    print("Multiple Cache Threads Single Server Thread")
    test = 'multi-cache-single-server-'
    file_loc = root + test

    webproxy_out = open(file_loc + 'webproxy.out', 'w')
    simplecached_out = open(file_loc + 'simplecached.out', 'w')
    gfclient_out = open(file_loc + 'gfclient.out', 'w')

    simplecached = Popen(['./simplecached', '-t 8'], shell=False, stdout=simplecached_out, stderr=simplecached_out)
    webproxy = Popen('./webproxy', shell=False, stdout=webproxy_out, stderr=webproxy_out)
    sleep(1)

    start = time()
    client = Popen('./gfclient_measure -p 6200 -t 16', shell=True, stdout=gfclient_out, stderr=gfclient_out)
    while client.poll() == None:
        if time() - start > 10:  # Give up after 10 seconds
            break
    end = time()
    print("Elapsed time {}".format(end - start))
    if client.poll() == None:
        print('Server taking too long to respond. Client probably hung.')
        client.send_signal(SIGINT)
    else:
        print('Client finished in {}. exit code: {}'.format(end-start, client.poll()))
    webproxy.terminate()
    simplecached.terminate()

def file_not_found_test():
    print("File Not Found Test")
    test = 'file-not-found-test-'
    file_loc = root + test
    webproxy_out = open(file_loc + 'webproxy.out', 'w')
    simplecached_out = open(file_loc + 'simplecached.out', 'w')
    gfclient_out = open(file_loc + 'gfclient.out', 'w')

    simplecached = Popen('./simplecached', shell=False, stdout=simplecached_out, stderr=simplecached_out)
    webproxy = Popen('./webproxy', shell=False, stdout=webproxy_out, stderr=webproxy_out)
    sleep(1)

    start = time()
    client = Popen('./gfclient_measure -p 6200 -t 16 -w file-not-found-workload.txt', shell=True, stdout=gfclient_out, stderr=gfclient_out)
    while client.poll() == None:
        if time() - start > 10:  # Give up after 10 seconds
            break
    end = time()
    print("Elapsed time {}".format(end - start))
    if client.poll() == None:
        print('Server taking too long to respond. Client probably hung.')
        client.send_signal(SIGINT)
    else:
        print('Client finished in {}. exit code: {}'.format(end-start, client.poll()))
    webproxy.terminate()
    simplecached.terminate()


def multiple_client_threads():
    print("Multiple Client Threads")
    test = 'multiple-client-threads'
    file_loc = root + test
    webproxy_out = open(file_loc + 'webproxy.out', 'w')
    simplecached_out = open(file_loc + 'simplecached.out', 'w')
    gfclient_out = open(file_loc + 'gfclient.out', 'w')

    simplecached = Popen('./simplecached', shell=False, stdout=simplecached_out, stderr=simplecached_out)
    webproxy = Popen('./webproxy', shell=False, stdout=webproxy_out, stderr=webproxy_out)
    sleep(1)

    start = time()
    client = Popen('./gfclient_measure -p 6200 -t 16', shell=True, stdout=gfclient_out, stderr=gfclient_out)
    while client.poll() == None:
        if time() - start > 10:  # Give up after 10 seconds
            break
    end = time()

    print("Elapsed time {}".format(end - start))
    if client.poll() == None:
        print('Server taking too long to respond. Client probably hung.')
        client.send_signal(SIGINT)
    else:
        print('Client finished in {}. exit code: {}'.format(end-start, client.poll()))
    webproxy.terminate()
    simplecached.terminate()

def simple_file_transfer():
    print("Simple File Transfer")
    test = 'simple-file-transfer-'
    file_loc = root + test
    webproxy_out = open(file_loc + 'webproxy.out', 'w')
    simplecached_out = open(file_loc + 'simplecached.out', 'w')
    gfclient_out = open(file_loc + 'gfclient.out', 'w')
    simplecached = Popen('./simplecached', shell=False, stdout=simplecached_out, stderr=simplecached_out)
    webproxy = Popen('./webproxy', shell=False, stdout=webproxy_out, stderr=webproxy_out)
    sleep(1)
    start = time()
    client = Popen('./gfclient_measure -p 6200 -t 1', shell=True, stdout=gfclient_out, stderr=gfclient_out)
    while client.poll() == None:
        if time() - start > 5:  # Give up after 10 seconds
            break
    end = time()
    print("Elapsed time {}".format(end - start))
    if client.poll() == None:
        print('Server taking too long to respond. Client probably hung.')
        client.send_signal(SIGINT)
    else:
        print('Client finished. exit code:', client.poll())
    webproxy.terminate()
    simplecached.terminate()

def main():
    call(['mkdir', '-p',  root])
    simple_file_transfer()
    sleep(0.5)
    multiple_client_threads()
    sleep(0.5)
    file_not_found_test()
    sleep(0.5)
    multi_cache_single_server()


if __name__=="__main__":
    main()
