function getHour(){
    return 10;
}

var populationChange_init = [{x: [5], y: [2], type: "bar"}];
if (populationChange_init[0]['x'].indexOf(getHour()) === -1) {
    console.log(populationChange_init[0]['x'].indexOf(getHour()));
    populationChange_init[0]['x'].push(getHour());
    console.log(populationChange_init);
    hourFirst = hourLast = 5;
    populationChange_init[0]['y'].push(0);

}
else {
    hourLast = 10;
}
hourLast = 10;
populationChange_init[0]['y'][populationChange_init[0]['x'].indexOf(getHour())] = hourLast - hourFirst;
console.log(populationChange_init[0]['x'].indexOf(getHour()));
console.log(populationChange_init);