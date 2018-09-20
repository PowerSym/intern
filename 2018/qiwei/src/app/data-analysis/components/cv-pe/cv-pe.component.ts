import { Component, OnInit, ViewChild, AfterViewInit, OnChanges, SimpleChanges, Input } from '@angular/core';
import { Router } from '@angular/router';
import { WholeGraphService } from '../../../app-shared/services/whole-graph.service';
import { ConfirmDialogModule, ConfirmationService } from 'primeng/primeng';
import { GraphManageService } from '../../../app-shared/services/graph-manage.service';
import { CrossValidService } from '../../services/cross-valid.service'

import * as gvis from 'gvis';
const GraphChart = (<any>gvis).GraphChart;

@Component({
  selector: 'app-cv-pe',
  templateUrl: './cv-pe.component.html',
  styleUrls: ['./cv-pe.component.css'],
  providers: [CrossValidService]
})
export class CvPeComponent implements OnInit, AfterViewInit {
  @ViewChild('lineContainer1') container2: any;
  public line1: any;
  @Input() sdate: Date;
  @Input() edate: Date;
  @Input() selectedmeter: number;
  selectedphase: any;
  phases: any;
  stats = [];
"chart size bug fix"
  constructor(    
    private router: Router,
    private wholeGraphService: WholeGraphService,
    private graphManageService: GraphManageService,
    private confirmationService: ConfirmationService,
    private crossvalid: CrossValidService
  ) { 
    this.stats  = [{'CC':0,'meand':0,'maxd':0}];
    this.phases = [{label:'All',value:'all'}];
    this.selectedphase = 'all';
  }

  ngOnInit() {
  }
  ngAfterViewInit(){
    this.line1 = new gvis.LineChart({
      render: {
        dataZoom:{
          show: true,
          top: 0,
          bottom: 350,
          handleSize: '20%'
        },
        container: this.container2.nativeElement,
        legend: {
            show: true
        },
        textStyle: {
            fontStyle: 'italic',
            fontWeight: 'bolder',
            fontFamily: '"Courier New", Courier, monospace',
            fontSize: 12,
        },
        title: {
            text: ''
        },
    },
    formatter:{
      xAxis: function(value, index){
        var options = {year:'numeric', month:'numeric', day:'numeric'}
        let time1 = new Date(value).toLocaleString('en-US',options)
        return time1
      }
    },
    data:{
    yAxis:{
      min:0
      // max:4
    }}
    }); 
    this.plotpe();
  }
  plotpe():void {
    this.crossvalid.p_e_com(this.selectedmeter,this.sdate.getTime(),this.edate.getTime(),this.selectedphase)
    .then(line=>{
      this.line1.reloadData(line[0]).update();
      this.stats = line[1];
    })
  }
  plotpe_reload(meter:number,startDate:Date,endDate:Date):void {
    this.crossvalid.p_e_com(meter,startDate.getTime(),endDate.getTime(),this.selectedphase)
    .then(line=>{
      this.line1.reloadData(line[0]).update();
      this.stats = line[1];
    })
  }
}
