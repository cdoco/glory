package message

import (
	"encoding/binary"
	"encoding/json"
	"io"
	"log"
	"unsafe"
)

const (
	//IntgerType is int type
	IntgerType = 1
	//StingType is sting type
	StingType = 2
	//MapType is map type
	MapType = 3
)

// Decode decodes a message from reader.
func Decode(r io.Reader) (name []byte, params []interface{}, err error) {

	byteChan := make(chan []byte)
	errorChan := make(chan error)

	go func(bc chan []byte, ec chan error) {
		lenData := make([]byte, 4)
		_, err := io.ReadFull(r, lenData)
		if err != nil {
			ec <- err
			close(bc)
			close(ec)
			log.Println("decode read message len exited")
			return
		}
		bc <- lenData
	}(byteChan, errorChan)

	var lenBytes []byte

	select {
	case err := <-errorChan:
		return nil, nil, err

	case lenBytes = <-byteChan:
		if lenBytes == nil {
			log.Printf("read type bytes nil\n")
		}
		l := binary.BigEndian.Uint32(lenBytes)

		if l > 1<<23 {
			log.Printf("message has bytes(%d) beyond max %d\n", l, 1<<23)
			return
		}

		//read data
		data := make([]byte, int(l))
		_, err = io.ReadFull(r, data)
		if err != nil {
			return nil, nil, err
		}

		n := 0

		//parse name
		l = binary.BigEndian.Uint32(data[n:4])
		n = n + 4
		nEnd := n + int(l)
		nameData := data[n:nEnd]
		n = nEnd

		var paramsData []interface{}
		if n != len(data) {
			//parse meta
			l = binary.BigEndian.Uint32(data[n : n+4])
			n = n + 4

			if l > 0 {
				paramsData, err = decodeParamsdata(l, data[n:])
				if err != nil {
					return nil, nil, err
				}
			}

		}

		return nameData, paramsData, err
	}
}

func decodeParamsdata(l uint32, data []byte) ([]interface{}, error) {
	var m []interface{}

	n := uint32(0)

	for n < l {

		//type
		t := binary.BigEndian.Uint32(data[n : n+4])
		n = n + 4

		//length
		sl := binary.BigEndian.Uint32(data[n : n+4])
		n = n + 4

		//value
		var v interface{}

		//intger type
		if t == IntgerType {
			v = sl
			sl = 0
		}

		//sting type
		if t == StingType {
			v = ByteToString(data[n : n+sl])
		}

		//map type
		if t == MapType {
			var ret map[string]interface{}
			if err := json.Unmarshal(data[n:n+sl], &ret); err != nil {
				return nil, err
			}
			v = ret
		}

		n = n + sl
		m = append(m, v)
	}

	return m, nil
}

//ByteToString byte to string
func ByteToString(b []byte) string {
	return *(*string)(unsafe.Pointer(&b))
}

//StringToByte string to byte
func StringToByte(s string) []byte {
	x := (*[2]uintptr)(unsafe.Pointer(&s))
	h := [3]uintptr{x[0], x[1], x[1]}
	return *(*[]byte)(unsafe.Pointer(&h))
}
