// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


namespace FunctionEGDWDumper
{
    using System;

    // TODO, move this class to a Contracts assembly that is shared across different projects
    class WindTurbineMeasure
    {
        public string DeviceId { get; set; }
        public DateTime MeasureTime { get; set; }
        public float GeneratedPower { get; set; }
        public float WindSpeed { get; set; }
        public float TurbineSpeed { get; set; }
    }
}