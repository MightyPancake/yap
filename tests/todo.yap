module hello {}

i32 fn add(i32 a, i32 b){
    ret a+b;
}

i32 fn main(){
    i32 a = ({
        i32 res = 1;
        res;
    });
    ret a - 1;
}