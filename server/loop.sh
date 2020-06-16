while true; do

    curl -I localhost:1234/test-file-1 -H "Expect:" -v >> /dev/null &

    curl -X POST localhost:1234/nothing -H "Expect:" -v &

    curl -X DELETE localhost:1234/nope -H "Expect:" -v &

    curl localhost:1234/test-file-2 -H "Expect:" -v >> /dev/null &

    curl localhost:1234/test-file-1 -H "Expect:" -v >> /dev/null &

    curl localhost:1234/test-file-3 -H "Expect:" -v >> /dev/null &

    curl localhost:1234/test-file-4 -H "Expect:" -v >> /dev/null &

    curl localhost:1234/test-file-5 -H "Expect:" -v >> /dev/null &

    curl localhost:1234/test-file-2 -H "Expect:" -v >> /dev/null &

    curl -T test-file-1 localhost:1234/test-file-1-result -v &

    curl -T test-file-2 localhost:1234/test-file-2-result -v &

    curl -T test-file-5 localhost:1234/test-file-5-result -v &

    curl -T test-file-4 localhost:1234/test-file-4-result -v &

    curl -T test-file-3 localhost:1234/test-file-3-result -v &

    curl -X POST localhost:1234/nothing -H "Expect:" -v &

    curl -X DELETE localhost:1234/nope -H "Expect:" -v &

    curl localhost:1234/test-file-3 -H "Expect:" -v >> /dev/null &

    curl localhost:1234/test-file-4 -H "Expect:" -v >> /dev/null &

    curl localhost:1234/test-file-5 -H "Expect:" -v >> /dev/null &

    curl localhost:1234/test-file-2 -H "Expect:" -v >> /dev/null &


    wait

done